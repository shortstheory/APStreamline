#include "GenericAdaptiveStreaming.h"
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>

// GenericAdaptiveStreaming::GenericAdaptiveStreaming() : camera_type(CameraType::RAW_CAM)
// {
//     g_warning("noparmctr");
// }

GenericAdaptiveStreaming::GenericAdaptiveStreaming(string _device, CameraType type) : device(_device), camera_type(type)
{
    g_warning("param ctr");
    if (camera_type == CameraType::RAW_CAM) {
        video_presets[ResolutionPresets::LOW] = "video/x-raw, width=(int)320, height=(int)240, framerate=(fraction)30/1";
        video_presets[ResolutionPresets::MED] = "video/x-raw, width=(int)640, height=(int)480, framerate=(fraction)30/1";
        video_presets[ResolutionPresets::HIGH] = "video/x-raw, width=(int)1280, height=(int)720, framerate=(fraction)30/1";
    }
    else if (camera_type == CameraType::H264_CAM) {
        video_presets[ResolutionPresets::LOW] = "video/x-h264, width=(int)320, height=(int)240, framerate=(fraction)30/1";
        video_presets[ResolutionPresets::MED] = "video/x-h264, width=(int)640, height=(int)480, framerate=(fraction)30/1";
        video_presets[ResolutionPresets::HIGH] = "video/x-h264, width=(int)1280, height=(int)720, framerate=(fraction)30/1";
    }

    bitrate_presets[ResolutionPresets::LOW] = 500;
    bitrate_presets[ResolutionPresets::MED] = 1500;
    bitrate_presets[ResolutionPresets::HIGH] = 3500;

    // if (link_all_elements()) {
    //     g_warning("goodlink");
    // } else {
    //     g_warning("bad link");
    // }
}

//unreffing pointers which are null can be dangerous, check this
GenericAdaptiveStreaming::~GenericAdaptiveStreaming()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    // gst_object_unref(v4l2_src);
    // gst_object_unref(video_udp_sink);
    // gst_object_unref(h264_encoder);
    // gst_object_unref(rtph264_payloader);
    // gst_object_unref(rtpbin);
    // gst_object_unref(rr_rtcp_identity);
    // gst_object_unref(sr_rtcp_identity);
}

bool GenericAdaptiveStreaming::init_elements()
{
    pipeline = gst_pipeline_new("adaptive-pipeline");
    src_capsfilter = gst_element_factory_make("capsfilter", NULL);

    // choose encoder according to ctr next time
    h264_parser = gst_element_factory_make("h264parse", NULL);
    v4l2_src = gst_element_factory_make("v4l2src", NULL);
    if (camera_type == CameraType::RAW_CAM) {
        h264_encoder = gst_element_factory_make("x264enc", NULL);
        videoconvert = gst_element_factory_make("videoconvert", NULL);
    }
    else if (camera_type == CameraType::H264_CAM) {
        // do nothing
    }

    rtph264_payloader = gst_element_factory_make("rtph264pay", "pay0");

    if (!pipeline && !src_capsfilter && !rtph264_payloader && !h264_parser && !v4l2_src) {
        if (camera_type == CameraType::RAW_CAM && !h264_encoder) {
            return false;
        }
    }
    return true;
}


void GenericAdaptiveStreaming::init_element_properties()
{
    set_resolution(ResolutionPresets::LOW);

    g_object_set(G_OBJECT(v4l2_src), "device", device.c_str(), NULL);
    if (camera_type == CameraType::RAW_CAM) {
        g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "threads", 4, NULL);
    }
    else if (camera_type == CameraType::H264_CAM) {
        // g_object_set(G_OBJECT(H264_CAM_src), "bitrate", 1000000, NULL);
    }
}

void GenericAdaptiveStreaming::pipeline_add_elements()
{
    gst_bin_add_many(GST_BIN(pipeline), v4l2_src, src_capsfilter, rtph264_payloader, h264_parser, NULL);
    if (camera_type == CameraType::RAW_CAM) {
        gst_bin_add_many(GST_BIN(pipeline), h264_encoder, videoconvert, NULL);
    }
    else if (camera_type == CameraType::H264_CAM) {
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

void GenericAdaptiveStreaming::adapt_stream()
{
    QoSReport qos_report = qos_estimator.get_qos_report();
    // adapt according to the information in this report
    if (qos_report.get_fraction_lost() == 0) {
        if (qos_report.get_encoding_bitrate() < qos_report.get_estimated_bitrate() * 1.5) {
            improve_quality();
        }
        else {
            g_warning("Buffer overflow possible!");
            degrade_quality();
        }
    }
    else {
        decrease_resolution();
    }
}

void GenericAdaptiveStreaming::improve_quality()
{
    set_encoding_bitrate(h264_bitrate+bitrate_inc);
    if (current_res == ResolutionPresets::LOW &&
        h264_bitrate > bitrate_presets[ResolutionPresets::MED]) {
        set_resolution(ResolutionPresets::MED);
    }
    else if (current_res == ResolutionPresets::MED &&
             h264_bitrate > bitrate_presets[ResolutionPresets::HIGH]) {
        set_resolution(ResolutionPresets::HIGH);
    }
}

void GenericAdaptiveStreaming::degrade_quality()
{
    set_encoding_bitrate(h264_bitrate-bitrate_dec);
    if (current_res == ResolutionPresets::HIGH &&
        h264_bitrate < bitrate_presets[ResolutionPresets::MED]) {
        set_resolution(ResolutionPresets::MED);
    }
    else if (current_res == ResolutionPresets::MED &&
             h264_bitrate < bitrate_presets[ResolutionPresets::LOW]) {
        set_resolution(ResolutionPresets::LOW);
    }
}

void GenericAdaptiveStreaming::set_encoding_bitrate(guint32 bitrate)
{
    if (bitrate >= min_bitrate && bitrate <= max_bitrate) {
        h264_bitrate = bitrate;
        if (camera_type == CameraType::RAW_CAM) {
            g_object_set(G_OBJECT(h264_encoder), "bitrate", bitrate, NULL);
        }
        else if (camera_type == CameraType::H264_CAM) {
            g_object_get(v4l2_src, "device-fd", &v4l2_cam_fd, NULL);
            if (v4l2_cam_fd > 0) {
                v4l2_control bitrate_ctrl;
                v4l2_control veritcal_flip;

                bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
                bitrate_ctrl.value = bitrate*1000;

                veritcal_flip.id = V4L2_CID_VFLIP;
                veritcal_flip.value = TRUE;

                if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1 ||
                    ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &veritcal_flip) == -1) {
                    g_warning("ioctl fail :/");
                }
            }
        }
    }
}

// presets
// LOW - 500kbps
// MED - 1500kbps
// HIGH - 3500kbps

void GenericAdaptiveStreaming::set_resolution(ResolutionPresets setting)
{
    g_warning("RES CHANGE! %d ", setting);

    string caps_filter_string;
    caps_filter_string = video_presets[setting];
    set_encoding_bitrate(bitrate_presets[setting]);

    current_res = setting;
    GstCaps* src_caps;

    src_caps = gst_caps_from_string(caps_filter_string.c_str());

    g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
    gst_caps_unref(src_caps);
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