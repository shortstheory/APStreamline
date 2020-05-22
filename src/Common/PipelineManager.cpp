#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include "PipelineManager.h"

PipelineManager::PipelineManager(string _device, int quality, CameraType type) : network_state(NetworkState::STEADY), successive_transmissions(0),
                                                                                 device(_device), quality(quality), camera_type(type)
{
    switch (camera_type) {
    case CameraType::MJPG_CAM:
        video_presets[ResolutionPresets::LOW] = MJPG_CAPS_FILTERS[VIDEO_320x240x30];
        video_presets[ResolutionPresets::MED] = MJPG_CAPS_FILTERS[VIDEO_640x480x30];
        video_presets[ResolutionPresets::HIGH] = MJPG_CAPS_FILTERS[VIDEO_1280x720x30];
        break;
    case CameraType::RPI_CAM:
    case CameraType::UVC_CAM:
        video_presets[ResolutionPresets::LOW] = H264_CAPS_FILTERS[VIDEO_320x240x30];
        video_presets[ResolutionPresets::MED] = H264_CAPS_FILTERS[VIDEO_640x480x30];
        video_presets[ResolutionPresets::HIGH] = H264_CAPS_FILTERS[VIDEO_1280x720x30];
        break;
    case CameraType::JETSON_CAM:
        video_presets[ResolutionPresets::LOW] = JETSON_CAPS_FILTERS[VIDEO_320x240x30];
        video_presets[ResolutionPresets::MED] = JETSON_CAPS_FILTERS[VIDEO_640x480x30];
        video_presets[ResolutionPresets::HIGH] = JETSON_CAPS_FILTERS[VIDEO_1280x720x30];
        break;
    }

    bitrate_presets[ResolutionPresets::LOW] = LOW_QUAL_BITRATE;
    bitrate_presets[ResolutionPresets::MED] = MED_QUAL_BITRATE;
    bitrate_presets[ResolutionPresets::HIGH] = HIGH_QUAL_BITRATE;

    pipeline = nullptr;
    camera = nullptr;
    src_capsfilter = nullptr;
    videoconvert = nullptr;
    h264_encoder = nullptr;
    h264_parser = nullptr;
    rtph264_payloader = nullptr;
    text_overlay = nullptr;
    tee = nullptr;
    multi_udp_sink = nullptr;

    set_state_constants();
}

int PipelineManager::get_quality_bitrate(int quality)
{
    if (quality == VIDEO_320x240x15 || quality == VIDEO_320x240x30 || quality == VIDEO_320x240x60) {
        return LOW_QUAL_BITRATE;
    } else if (quality == VIDEO_640x480x15 || quality == VIDEO_640x480x30 || quality == VIDEO_640x480x60) {
        return MED_QUAL_BITRATE;
    } else if (quality == VIDEO_1280x720x15 || quality == VIDEO_1280x720x30 || quality == VIDEO_1280x720x60) {
        return HIGH_QUAL_BITRATE;
    }
    return LOW_QUAL_BITRATE;
}

void PipelineManager::set_state_constants()
{
    if (network_state == NetworkState::STEADY) {
        MAX_BITRATE = MAX_STEADY_BITRATE;
        MIN_BITRATE = MIN_STEADY_BITRATE;
        INC_BITRATE = INC_STEADY_BITRATE;
        DEC_BITRATE = DEC_STEADY_BITRATE;
    } else if (network_state == NetworkState::CONGESTION) {
        MAX_BITRATE = MAX_CONGESTION_BITRATE;
        MIN_BITRATE = MIN_CONGESTION_BITRATE;
        INC_BITRATE = INC_CONGESTION_BITRATE;
        DEC_BITRATE = DEC_CONGESTION_BITRATE;
    }
}

void PipelineManager::adapt_stream()
{
    QoSReport qos_report;
    qos_report = qos_estimator.get_qos_report();

    if (successive_transmissions >= SUCCESSFUL_TRANSMISSION) {
        network_state = NetworkState::STEADY;
    }

    set_state_constants();

    // If we get a number of transmissions without any packet loss, we can increase bitrate
    if (qos_report.fraction_lost == 0) {
        successive_transmissions++;
        if (multi_udp_sink) {
            if (qos_report.encoding_bitrate < qos_report.estimated_bitrate * 1.5) {
                improve_quality();
            } else {
                cerr << "Buffer overflow possible!" << endl;
                degrade_quality();
            }
        } else {
            improve_quality();
        }
    } else {
        network_state = NetworkState::CONGESTION;
        successive_transmissions = 0;
        degrade_quality();
    }
}

void PipelineManager::improve_quality()
{
    set_encoding_bitrate(h264_bitrate + INC_BITRATE);
    if (current_res == ResolutionPresets::LOW &&
        h264_bitrate > bitrate_presets[ResolutionPresets::MED]) {
        set_resolution(ResolutionPresets::MED);
    } else if (current_res == ResolutionPresets::MED &&
               h264_bitrate > bitrate_presets[ResolutionPresets::HIGH]) {
        set_resolution(ResolutionPresets::HIGH);
    }
}

void PipelineManager::degrade_quality()
{
    set_encoding_bitrate(h264_bitrate - DEC_BITRATE);
    if (current_res == ResolutionPresets::HIGH &&
        h264_bitrate < bitrate_presets[ResolutionPresets::MED]) {
        set_resolution(ResolutionPresets::MED);
    } else if (current_res == ResolutionPresets::MED &&
               h264_bitrate < bitrate_presets[ResolutionPresets::LOW]) {
        set_resolution(ResolutionPresets::LOW);
    }
}

void PipelineManager::set_encoding_bitrate(guint32 bitrate)
{
    string currstate = (network_state == NetworkState::STEADY) ? "STEADY" : "CONGESTED";

    if (bitrate >= MIN_BITRATE && bitrate <= MAX_BITRATE) {
        h264_bitrate = bitrate;
    } else if (h264_bitrate > MAX_BITRATE) {
        h264_bitrate = MAX_BITRATE;
    } else if (h264_bitrate < MIN_BITRATE) {
        h264_bitrate = MIN_BITRATE;
    }

    switch (camera_type) {
    case MJPG_CAM:
        if (h264_encoder) {
            g_object_set(G_OBJECT(h264_encoder), "bitrate", h264_bitrate, NULL);
        }
        if (text_overlay) {
            QoSReport qos_report = qos_estimator.get_qos_report();

            string state = (network_state == NetworkState::STEADY) ? "Steady" : "Congested";
            string stats = "Bitrate: "
                           + to_string((long)qos_report.encoding_bitrate)
                           + " kbps Network: " + to_string((long)qos_report.estimated_bitrate)
                           + " kbps State: "
                           + state;
            g_object_set(G_OBJECT(text_overlay), "text", stats.c_str(), NULL);
        }
        break;
    case RPI_CAM:
        int v4l2_cam_fd;
        g_object_get(camera, "device-fd", &v4l2_cam_fd, NULL);
        if (v4l2_cam_fd > 0) {
            v4l2_control bitrate_ctrl;
            bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            bitrate_ctrl.value = h264_bitrate*1000;
            if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1) {
                cerr << "Camera does not support the IOCTL" << endl;
            }
        }
        break;
    case UVC_CAM:
        g_object_set(camera, "average-bitrate", h264_bitrate*1000, NULL);
        break;
    case JETSON_CAM:
        if (h264_encoder) {
            g_object_set(G_OBJECT(h264_encoder), "bitrate", h264_bitrate*1000, NULL);
        }
        break;
    };
}

// Swap out capsfilters for changing the resolution. This doesn't work with UVC/CSI cameras.

void PipelineManager::set_resolution(ResolutionPresets setting)
{
    if (camera_type != UVC_CAM && camera_type != JETSON_CAM) {
        string caps_filter_string;
        caps_filter_string = video_presets[setting];
        set_encoding_bitrate(bitrate_presets[setting]);
        current_res = setting;
        GstCaps* src_caps;
        src_caps = gst_caps_from_string(caps_filter_string.c_str());
        g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
        gst_caps_unref(src_caps);
    }
}

bool PipelineManager::get_element_references()
{
    if (pipeline) {
        tee = gst_bin_get_by_name(GST_BIN(pipeline), "tee_element");
        rtph264_payloader = gst_bin_get_by_name(GST_BIN(pipeline), "pay0");
        camera = gst_bin_get_by_name(GST_BIN(pipeline), "src");
        src_capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");

        switch (camera_type) {
        case MJPG_CAM:
            h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), "x264enc");
            text_overlay = gst_bin_get_by_name(GST_BIN(pipeline), "textoverlay");
            if (tee && rtph264_payloader && camera && src_capsfilter && h264_encoder && text_overlay) {
                g_object_set(G_OBJECT(text_overlay),
                             "valignment", 2,
                             "halignment", 0,
                             "font-desc", "Sans, 8", NULL);
                g_object_set(G_OBJECT(h264_encoder),
                             "tune", 0x00000004,
                             "threads", 4,
                             "key-int-max", I_FRAME_INTERVAL,
                             // intra-refresh breaks an iframe over multiple frames
                             "intra-refresh", TRUE,
                             NULL);
                return true;
            } else {
                return false;
            }
        case RPI_CAM:
            int v4l2_cam_fd;
            if (camera) {
                g_object_get(camera, "device-fd", &v4l2_cam_fd, NULL);
                if (v4l2_cam_fd > 0) {
                    v4l2_control vertical_flip;
                    vertical_flip.id = V4L2_CID_VFLIP;
                    vertical_flip.value = TRUE;

                    v4l2_control horizontal_flip;
                    horizontal_flip.id = V4L2_CID_HFLIP;
                    horizontal_flip.value = TRUE;

                    v4l2_control i_frame_interval;
                    i_frame_interval.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
                    i_frame_interval.value = I_FRAME_INTERVAL;

                    if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &vertical_flip) == -1 ||
                        ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &horizontal_flip) == -1 ||
                        ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &i_frame_interval) == -1) {
                        return false;
                    }
                    return true;
                }
            }
        case UVC_CAM:
            if (tee && rtph264_payloader && camera && src_capsfilter) {
                return true;
            } else {
                return false;
            }
        case JETSON_CAM:
            h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), "omxh264enc");
            if (rtph264_payloader && camera && src_capsfilter && h264_encoder) {
                return true;
            } else {
                return false;
            }
        };
    }
    return false;
}

int PipelineManager::get_quality()
{
    return quality;
}

void PipelineManager::set_quality(int _quality)
{
    quality = _quality;
    if (quality == AUTO_PRESET) {
        network_state = STEADY;
        set_state_constants();
        h264_bitrate = MIN_BITRATE;
        set_resolution(ResolutionPresets::LOW);
        successive_transmissions = 0;
    } else {
        // Set the bitrate to the presets we use in AUTO mode
        h264_bitrate = get_quality_bitrate(quality);
        if (camera != nullptr) {
            string caps_filter_string;
            GstCaps* src_caps;
            switch(camera_type) {
            case CameraType::MJPG_CAM:
                g_object_set(G_OBJECT(h264_encoder), "bitrate", h264_bitrate, NULL);
                caps_filter_string = MJPG_CAPS_FILTERS[quality];
                src_caps = gst_caps_from_string(caps_filter_string.c_str());
                g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
                break;
            case CameraType::RPI_CAM:
                int v4l2_cam_fd;
                g_object_get(camera, "device-fd", &v4l2_cam_fd, NULL);
                if (v4l2_cam_fd > 0)
                {
                    v4l2_control bitrate_ctrl;
                    v4l2_control i_frame_interval;

                    bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
                    bitrate_ctrl.value = h264_bitrate * 1000;

                    i_frame_interval.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
                    i_frame_interval.value = 60;

                    if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1 ||
                        ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &i_frame_interval) == -1)
                    {
                        cerr << "Camera does not support the IOCTL" << endl;
                    }
                }
                caps_filter_string = H264_CAPS_FILTERS[quality];
                src_caps = gst_caps_from_string(caps_filter_string.c_str());
                g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
                break;
            case CameraType::UVC_CAM:
                break;
            case CameraType::JETSON_CAM:
                break;
            }
        }
    }
}

string PipelineManager::get_device()
{
    return device;
}

CameraType PipelineManager::get_camera_type()
{
    return camera_type;
}