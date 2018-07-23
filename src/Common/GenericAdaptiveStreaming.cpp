#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>

#include "GenericAdaptiveStreaming.h"

// not sure why this isn't included in some pacakges?!
#ifndef V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME
#define V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME		(V4L2_CID_MPEG_BASE+229)
#endif

GenericAdaptiveStreaming::GenericAdaptiveStreaming(string _device, CameraType type) :
    device(_device), camera_type(type),
    network_state(NetworkState::STEADY), successive_transmissions(0)
{
    if (camera_type == CameraType::RAW_CAM) {
        video_presets[ResolutionPresets::LOW] = RAW_CAPS_FILTERS[VIDEO_320x240x30];
        video_presets[ResolutionPresets::MED] = RAW_CAPS_FILTERS[VIDEO_640x480x30];
        video_presets[ResolutionPresets::HIGH] = RAW_CAPS_FILTERS[VIDEO_1280x720x30];
    } else if (camera_type == CameraType::H264_CAM || camera_type == CameraType::UVC_CAM) {
        video_presets[ResolutionPresets::LOW] = H264_CAPS_FILTERS[VIDEO_320x240x30];
        video_presets[ResolutionPresets::MED] = H264_CAPS_FILTERS[VIDEO_640x480x30];
        video_presets[ResolutionPresets::HIGH] = H264_CAPS_FILTERS[VIDEO_1280x720x30];
    }

    bitrate_presets[ResolutionPresets::LOW] = LOW_QUAL_BITRATE;
    bitrate_presets[ResolutionPresets::MED] = MED_QUAL_BITRATE;
    bitrate_presets[ResolutionPresets::HIGH] = HIGH_QUAL_BITRATE;

    set_state_constants();
    text_overlay = NULL;
}

//unreffing pointers which are null can be dangerous, check this
GenericAdaptiveStreaming::~GenericAdaptiveStreaming()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void GenericAdaptiveStreaming::pipeline_add_elements()
{
    gst_bin_add_many(GST_BIN(pipeline),
                     v4l2_src,src_capsfilter,
                     text_overlay,
                     rtph264_payloader,
                     h264_parser,
                     NULL);
    if (camera_type == CameraType::RAW_CAM) {
        gst_bin_add_many(GST_BIN(pipeline), h264_encoder, videoconvert, NULL);
    } else if (camera_type == CameraType::H264_CAM) {
    }
}

bool GenericAdaptiveStreaming::play_pipeline()
{
    return gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

bool GenericAdaptiveStreaming::pause_pipeline()
{
    return gst_element_set_state(pipeline, GST_STATE_PAUSED);
}

GstBus* GenericAdaptiveStreaming::get_pipeline_bus()
{
    return gst_element_get_bus(pipeline);
}

void GenericAdaptiveStreaming::set_state_constants()
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

void GenericAdaptiveStreaming::adapt_stream()
{
    QoSReport qos_report = qos_estimator.get_qos_report();
    // adapt according to the information in this report
    if (successive_transmissions >= SUCCESSFUL_TRANSMISSION) {
        network_state = NetworkState::STEADY;
    }

    set_state_constants();

    if (qos_report.fraction_lost == 0) {
        successive_transmissions++;
        if (multi_udp_sink) {
            if (qos_report.encoding_bitrate < qos_report.estimated_bitrate * 1.5) {
                improve_quality();
            } else {
                g_warning("Buffer overflow possible!");
                degrade_quality();
            }
        } else {
            improve_quality();
        }
    } else {
        network_state = NetworkState::CONGESTION;
        successive_transmissions = 0;
        decrease_resolution();
        // Force key-frame on hardware encoders. Doesn't always work if the IOCTL isn't supported
        if (camera_type == CameraType::H264_CAM) {
            int v4l2_cam_fd;
            g_object_get(v4l2_src, "device-fd", &v4l2_cam_fd, NULL);
            v4l2_control force_keyframe;
            force_keyframe.id = V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME; //value is ignored
            // if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &force_keyframe) == -1) {
            //     g_warning("Keyframe ioctl failed");
            // } else {
            //     g_warning("New keyframe requested");
            // }
        }
    }
}

void GenericAdaptiveStreaming::improve_quality()
{
    set_encoding_bitrate(h264_bitrate + INC_BITRATE);
    if (current_res == ResolutionPresets::LOW &&
        h264_bitrate > bitrate_presets[ResolutionPresets::MED]) {
        g_warning("Low->MED");
        set_resolution(ResolutionPresets::MED);
    } else if (current_res == ResolutionPresets::MED &&
               h264_bitrate > bitrate_presets[ResolutionPresets::HIGH]) {
        g_warning("Med->HIGH");
        set_resolution(ResolutionPresets::HIGH);
    }
}

void GenericAdaptiveStreaming::degrade_quality()
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

void GenericAdaptiveStreaming::set_encoding_bitrate(guint32 bitrate)
{
    string currstate = (network_state == NetworkState::STEADY) ? "STEADY" : "CONGESTED";
    // g_warning("Curr state %s %d", currstate.c_str(), successive_transmissions);

    if (bitrate >= MIN_BITRATE && bitrate <= MAX_BITRATE) {
        h264_bitrate = bitrate;
    } else if (h264_bitrate > MAX_BITRATE) {
        h264_bitrate = MAX_BITRATE;
    } else if (h264_bitrate < MIN_BITRATE) {
        h264_bitrate = MIN_BITRATE;
    }

    switch (camera_type) {
    case RAW_CAM:
        if (h264_encoder) {
            // g_warning("Bitrate val - %u", h264_bitrate);
            g_object_set(G_OBJECT(h264_encoder), "bitrate", h264_bitrate, NULL);
        }
        if (text_overlay) {
            QoSReport qos_report = qos_estimator.get_qos_report();

            string state = (network_state == NetworkState::STEADY) ? "STEADY" : "CONGESTED";
            string stats = "BR: " + to_string(h264_bitrate) + " H264: " +
                           to_string(qos_report.encoding_bitrate)
                           + " BW: " + to_string(qos_report.estimated_bitrate) + " STATE: " + state;

            g_object_set(G_OBJECT(text_overlay), "text", stats.c_str(), NULL);
        }
        break;
    case H264_CAM:
        int v4l2_cam_fd;
        g_object_get(v4l2_src, "device-fd", &v4l2_cam_fd, NULL);
        if (v4l2_cam_fd > 0) {
            v4l2_control bitrate_ctrl;
            v4l2_control veritcal_flip;
            v4l2_control horizontal_flip;
            v4l2_control i_frame_interval;

            bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
            bitrate_ctrl.value = h264_bitrate*1000;

            veritcal_flip.id = V4L2_CID_VFLIP;
            veritcal_flip.value = TRUE;

            horizontal_flip.id = V4L2_CID_HFLIP;
            horizontal_flip.value = TRUE;

            i_frame_interval.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
            i_frame_interval.value = I_FRAME_INTERVAL;

            if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1 ||
                ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &veritcal_flip) == -1 ||
                ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &horizontal_flip) == -1 ||
                ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &i_frame_interval) == -1) {
                g_warning("ioctl fail :/");
            }
        }
        break;
    case UVC_CAM:
        g_object_set(v4l2_src, "average-bitrate", h264_bitrate*1000, NULL);
        break;
    };
}

void GenericAdaptiveStreaming::set_resolution(ResolutionPresets setting)
{
    if (camera_type != UVC_CAM) {
        string caps_filter_string;
        caps_filter_string = video_presets[setting];
        set_encoding_bitrate(bitrate_presets[setting]);
        g_warning("RES CHANGE! %d %s", setting, caps_filter_string.c_str());

        current_res = setting;

        GstCaps* src_caps;
        src_caps = gst_caps_from_string(caps_filter_string.c_str());
        g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
        gst_caps_unref(src_caps);
    }
}

void GenericAdaptiveStreaming::increase_resolution()
{
    switch (current_res) {
    case ResolutionPresets::LOW:
        set_resolution(ResolutionPresets::MED);
        break;
    case ResolutionPresets::MED:
        set_resolution(ResolutionPresets::HIGH);
        break;
    default:
        break;
    }
}

void GenericAdaptiveStreaming::decrease_resolution()
{
    switch (current_res) {
    case ResolutionPresets::MED:
        set_resolution(ResolutionPresets::LOW);
        break;
    case ResolutionPresets::HIGH:
        set_resolution(ResolutionPresets::MED);
        break;
    default:
        break;
    }
}

bool GenericAdaptiveStreaming::change_source(string _device)
{
    if (pause_pipeline() && gst_bin_remove(GST_BIN(pipeline), v4l2_src)) {
        v4l2_src = gst_element_factory_make("v4l2src", NULL);
        if (v4l2_src) {
            device = _device;
            g_object_set(G_OBJECT(v4l2_src), "device", _device.c_str(), NULL);
            gst_bin_add(GST_BIN(pipeline), v4l2_src);
            if (gst_element_link(v4l2_src, src_capsfilter)) {
                return play_pipeline();
            }
        }
    }
    return false;
}

void GenericAdaptiveStreaming::change_quality_preset(int quality)
{
    g_warning("Changing quality!");
    current_quality = quality;
    if (current_quality == AUTO_PRESET) {
        network_state = STEADY;
        set_state_constants();
        h264_bitrate = MIN_BITRATE;
        set_resolution(ResolutionPresets::LOW);
        successive_transmissions = 0;
    } else {
        string caps_filter_string;
        GstCaps* src_caps;
        // guint32 h264_bitrate;
        if (quality == VIDEO_320x240x15 || quality == VIDEO_320x240x30 || quality == VIDEO_320x240x60) {
            h264_bitrate = LOW_QUAL_BITRATE;
        } else if (quality == VIDEO_640x480x15 || quality == VIDEO_640x480x30 || quality == VIDEO_640x480x60) {
            h264_bitrate = MED_QUAL_BITRATE;
        } else if (quality == VIDEO_1280x720x15 || quality == VIDEO_1280x720x30 || quality ==VIDEO_1280x720x60) {
            h264_bitrate = HIGH_QUAL_BITRATE;
        }

        if (camera_type == CameraType::RAW_CAM) {
            // set it to x264enc defaults
            g_object_set(G_OBJECT(h264_encoder), "bitrate", h264_bitrate, NULL);

            caps_filter_string = RAW_CAPS_FILTERS[current_quality];
            src_caps = gst_caps_from_string(caps_filter_string.c_str());
            g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
        } else if (camera_type == CameraType::H264_CAM) {
            int v4l2_cam_fd;
            g_object_get(v4l2_src, "device-fd", &v4l2_cam_fd, NULL);
            if (v4l2_cam_fd > 0) {
                v4l2_control bitrate_ctrl;
                v4l2_control i_frame_interval;

                bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
                bitrate_ctrl.value = h264_bitrate*1000;

                i_frame_interval.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
                i_frame_interval.value = 60;

                if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1 ||
                    ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &i_frame_interval) == -1) {
                    g_warning("ioctl fail :/");
                }
            }
            caps_filter_string = H264_CAPS_FILTERS[current_quality];
            src_caps = gst_caps_from_string(caps_filter_string.c_str());
            g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
        }
    }
}