#include "AdaptiveStreaming.h" 
#include <functional>

// const string AdaptiveStreaming::receiver_ip_addr = "192.168.0.102";
const string AdaptiveStreaming::receiver_ip_addr = "127.0.0.1";
// const string AdaptiveStreaming::receiver_ip_addr = "10.42.0.56";

AdaptiveStreaming::AdaptiveStreaming()
{
    h264_bitrate = 5000;
    init_elements();
    init_element_properties();
    pipeline_add_elements();
    if(link_all_elements()) {
        g_warning("goodlink");
    } else {
        g_warning("bad link");
    }
}

//unreffing pointers which are null can be dangerous, check this
AdaptiveStreaming::~AdaptiveStreaming()
{
    // gst_object_unref(pipeline);
    // gst_object_unref(v4l2_src);
    // gst_object_unref(video_udp_sink);
    // gst_object_unref(h264_encoder);
    // gst_object_unref(rtph264_payloader);
    // gst_object_unref(rtpbin);
    // gst_object_unref(rr_rtcp_identity);
    // gst_object_unref(sr_rtcp_identity);

    // gst_caps_unref(video_caps);
    // gst_caps_unref(rtcp_caps);
}

bool AdaptiveStreaming::init_elements()
{
    pipeline = gst_pipeline_new ("adaptive-pipeline");
    v4l2_src = gst_element_factory_make ("v4l2src", NULL);
    src_capsfilter = gst_element_factory_make("capsfilter", NULL);
    // choose encoder according to ctr next time
    h264_encoder = gst_element_factory_make("x264enc", NULL);
    h264_parser = gst_element_factory_make("h264parse", NULL);
    rtph264_payloader = gst_element_factory_make("rtph264pay", NULL);
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rtp_identity = gst_element_factory_make("identity", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!pipeline && !v4l2_src && !src_capsfilter && !h264_encoder && !h264_parser && !rtph264_payloader && !rtpbin && !rr_rtcp_identity
        && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink && !rtcp_udp_sink && !rtp_identity) {
            return false;
    }
    return true;
}

void AdaptiveStreaming::init_element_properties()
{
    GstCaps* src_caps; 
    src_caps = gst_caps_new_simple ("video/x-raw",
                            "width", G_TYPE_INT, 1280,
                            "height", G_TYPE_INT, 720,
                            "framerate", GST_TYPE_FRACTION, 30, 1,
                            NULL);

    g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
    gst_caps_unref(src_caps);
    g_object_set(G_OBJECT(v4l2_src), "device", "/dev/video0", NULL);
    g_object_set(G_OBJECT(rtcp_udp_src), "caps", gst_caps_from_string("application/x-rtcp"), 
                        "port", rtcp_src_port, NULL);
    g_object_set(G_OBJECT(rtpbin), "latency", 0, NULL);
    g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "bitrate", h264_bitrate, "threads", 4, NULL);
    g_object_set(G_OBJECT(video_udp_sink), "host", receiver_ip_addr.c_str(), 
                        "port", video_sink_port, NULL);
    g_object_set(G_OBJECT(rtcp_udp_sink), "host", receiver_ip_addr.c_str(), 
                        "port", rtcp_sink_port, NULL);
}

void AdaptiveStreaming::pipeline_add_elements()
{
    gst_bin_add_many(GST_BIN(pipeline), v4l2_src, src_capsfilter, h264_encoder, h264_parser, rtph264_payloader,
                    rtpbin, rtp_identity, rr_rtcp_identity, sr_rtcp_identity, video_udp_sink,
                    rtcp_udp_sink, rtcp_udp_src, NULL);
}

bool AdaptiveStreaming::link_all_elements()
{
    // first link all the elements with autoplugging
    if (!(gst_element_link_many(v4l2_src, src_capsfilter, h264_encoder, h264_parser, rtph264_payloader, NULL) &&
        gst_element_link(rtcp_udp_src, rr_rtcp_identity))) {
        return false;
    }
    if(!gst_pad_link(gst_element_get_static_pad(rr_rtcp_identity,"src"), gst_element_get_request_pad(rtpbin, "recv_rtcp_sink_%u"))
        && !gst_pad_link(gst_element_get_static_pad(rtph264_payloader,"src"), gst_element_get_request_pad(rtpbin, "send_rtp_sink_%u"))
        && !gst_pad_link(gst_element_get_request_pad(rtpbin, "send_rtcp_src_%u"), gst_element_get_static_pad(sr_rtcp_identity,"sink"))
        && !gst_pad_link(gst_element_get_static_pad(sr_rtcp_identity,"src"), gst_element_get_static_pad(rtcp_udp_sink,"sink"))) {
        gst_element_link_many(rtpbin, rtp_identity, video_udp_sink, NULL);
        g_signal_connect(rtp_identity, "handoff", G_CALLBACK(static_rtp_callback), this);
        g_signal_connect(rr_rtcp_identity, "handoff", G_CALLBACK(static_callback), this);
        g_signal_connect(sr_rtcp_identity, "handoff", G_CALLBACK(static_callback), this);

        //setup callbacks here
        return true;
    }
    return false;
}

bool AdaptiveStreaming::start_playing()
{
    return gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

GstBus* AdaptiveStreaming::get_pipeline_bus()
{
    return gst_element_get_bus(pipeline);
}

void AdaptiveStreaming::static_callback(GstElement *src, GstBuffer *buf, gpointer data)
{
    if (data != nullptr) {
        AdaptiveStreaming* ptr = (AdaptiveStreaming*)data;
        ptr->rtcp_callback(src, buf);
    }
    // g_warning("Received rtcp");
    // ptr->rtcp_callback(src, buf, data);
}

void AdaptiveStreaming::static_rtp_callback(GstElement* src, GstBuffer* buf, gpointer data)
{
    if (data != nullptr) {
        AdaptiveStreaming* ptr = (AdaptiveStreaming*)data;
        ptr->rtp_callback(src, buf);
    }
}

void AdaptiveStreaming::rtp_callback(GstElement* src, GstBuffer* buf)
{
    guint32 buffer_size;
    buffer_size = gst_buffer_get_size(buf);
    qos_estimator.estimate_rtp_pkt_size(buffer_size);
    qos_estimator.estimate_encoding_rate(buffer_size);
}

void AdaptiveStreaming::rtcp_callback(GstElement* src, GstBuffer* buf)
{
    // g_warning("BuffSize: %lu", gst_buffer_get_size(buf));
    // find the right way around using mallocs
    GstRTCPBuffer *rtcp_buffer = (GstRTCPBuffer*)malloc(sizeof(GstRTCPBuffer));
    rtcp_buffer->buffer = NULL;
    gst_rtcp_buffer_map(buf, GST_MAP_READ, rtcp_buffer);
    GstRTCPPacket *packet = (GstRTCPPacket*)malloc(sizeof(GstRTCPPacket));
    gboolean more = gst_rtcp_buffer_get_first_packet(rtcp_buffer, packet);
    //same buffer can have an SDES and an RTCP pkt
    while (more) {
        qos_estimator.handle_rtcp_packet(packet);
        adapt_stream();
        more = gst_rtcp_packet_move_to_next(packet);
    }
    free(rtcp_buffer);
    free(packet);
}

void AdaptiveStreaming::adapt_stream()
{
    QoSReport qos_report = qos_estimator.get_qos_report();
    // adapt according to the information in this report

    GstCaps* src_caps; 
    src_caps = gst_caps_new_simple ("video/x-raw",
                            "width", G_TYPE_INT, 320,
                            "height", G_TYPE_INT, 180,
                            "framerate", GST_TYPE_FRACTION, 30, 1,
                            NULL);

    g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
    gst_caps_unref(src_caps);
}