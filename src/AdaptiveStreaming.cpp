#include "AdaptiveStreaming.h" 

const string AdaptiveStreaming::receiver_ip_addr = "127.0.0.1";

AdaptiveStreaming::AdaptiveStreaming()
{
    h264_bitrate = 2000;
    init_elements();
    init_caps(1280, 720, 30);
    init_element_properties();
}

//unreffing pointers if not null can be dangerous, check this
AdaptiveStreaming::~AdaptiveStreaming()
{
    gst_object_unref(pipeline);
    gst_object_unref(v4l2_src);
    gst_object_unref(video_udp_sink);
    gst_object_unref(h264_encoder);
    gst_object_unref(rtph264_payloader);
    gst_object_unref(rtpbin);
    gst_object_unref(rr_rtcp_identity);
    gst_object_unref(sr_rtcp_identity);

    gst_caps_unref(video_caps);
    // gst_caps_unref(rtcp_caps);
}

bool AdaptiveStreaming::init_elements()
{
    pipeline = gst_pipeline_new ("adaptive-pipeline");
    v4l2_src = gst_element_factory_make ("v4l2src", NULL);
    // choose encoder according to ctr next time
    h264_encoder = gst_element_factory_make("x264enc", NULL);
    h264_parser = gst_element_factory_make("h264parse", NULL);
    rtph264_payloader = gst_element_factory_make("rtph264pay", NULL);
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!pipeline && !v4l2_src && !h264_encoder && !h264_parser && !rtph264_payloader && !rtpbin && !rr_rtcp_identity
        && !sr_rtcp_identity && !rtcp_src && !video_udp_sink && !rtcp_udp_sink) {
            return false;
    }
    return true;
}

bool AdaptiveStreaming::init_caps(int width, int height, int framerate)
{
    video_caps = gst_caps_new_simple ("video/x-raw",
                                "width", G_TYPE_INT, width,
                                "height", G_TYPE_INT, height,
                                "framerate", GST_TYPE_FRACTION, framerate, 1,
                                NULL);
    return (video_caps != NULL) ? true : false;
}

void AdaptiveStreaming::init_element_properties()
{
    g_object_set(G_OBJECT(v4l2_src), "device", "/dev/video0", NULL);
    g_object_set(G_OBJECT(rtcp_src), "caps", gst_caps_from_string("application/x-rtcp"), "port", rtcp_src_port, NULL);
    g_object_set(G_OBJECT(rtpbin), "latency", 0, NULL);
    g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "bitrate", h264_bitrate, NULL);

    g_object_set(G_OBJECT(video_udp_sink), "host", receiver_ip_addr.c_str(), "port", video_sink_port, NULL);
    g_object_set(G_OBJECT(rtcp_udp_sink), "host", receiver_ip_addr.c_str(), "port", rtcp_sink_port, NULL);
}

bool link_all_elements()
{
    return true;
}