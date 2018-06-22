#include "UDPAdaptiveStreaming.h"

UDPAdaptiveStreaming::UDPAdaptiveStreaming(string _device, CameraType type, string _ip_addr, 
                                            gint _video_port, gint _rtcp_port) : 
                                            receiver_ip_addr(_ip_addr), video_sink_port(_video_port), 
                                            rtcp_port(_rtcp_port), GenericAdaptiveStreaming(_device, type)
{
    init_elements();
    init_element_properties();
    pipeline_add_elements();

    init_rtp_elements();
    init_rtp_element_properties();
    pipeline_add_rtp_elements();
    link_all_elements();
}

UDPAdaptiveStreaming::~UDPAdaptiveStreaming()
{
}

bool UDPAdaptiveStreaming::init_rtp_elements()
{
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rtp_identity = gst_element_factory_make("identity", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!rtpbin && !rr_rtcp_identity && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink && 
        !rtcp_udp_sink && !rtp_identity) {
        return false;
    }
    return true;
}

void UDPAdaptiveStreaming::init_rtp_element_properties()
{
    g_object_set(G_OBJECT(rtpbin), "latency", 0, NULL);
    g_object_set(G_OBJECT(video_udp_sink), "host", receiver_ip_addr.c_str(),
                 "port", video_sink_port, NULL);
    g_object_set(G_OBJECT(rtcp_udp_sink), "host", receiver_ip_addr.c_str(),
                 "port", rtcp_port, NULL);
    g_object_set(G_OBJECT(rtcp_udp_src), "caps", gst_caps_from_string("application/x-rtcp"),
                 "port", rtcp_port, NULL);
}

void UDPAdaptiveStreaming::pipeline_add_rtp_elements()
{
    gst_bin_add_many(GST_BIN(pipeline), rtpbin, rtp_identity, rr_rtcp_identity, 
                    sr_rtcp_identity, video_udp_sink, rtcp_udp_sink, rtcp_udp_src, NULL);
}

bool UDPAdaptiveStreaming::link_all_elements()
{
    // first link all the elements with autoplugging
    if (camera_type == CameraType::RAW_CAM) {
        if (!(gst_element_link_many(v4l2_src, src_capsfilter, videoconvert, h264_encoder, h264_parser, rtph264_payloader, NULL) &&
              gst_element_link(rtcp_udp_src, rr_rtcp_identity))) {
            return false;
        }
    }
    else if (camera_type == CameraType::H264_CAM) {
        if (!(gst_element_link_many(v4l2_src, src_capsfilter, h264_parser, rtph264_payloader, NULL) &&
              gst_element_link(rtcp_udp_src, rr_rtcp_identity))) {
            return false;
        }
    }
    if (!gst_pad_link(gst_element_get_static_pad(rr_rtcp_identity,"src"), gst_element_get_request_pad(rtpbin, "recv_rtcp_sink_%u"))
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

void UDPAdaptiveStreaming::static_callback(GstElement *src, GstBuffer *buf, gpointer data)
{
    if (data != nullptr) {
        UDPAdaptiveStreaming* ptr = (UDPAdaptiveStreaming*)data;
        g_warning("h264val - %d", ptr->h264_bitrate);
        ptr->rtcp_callback(src, buf);
    }
    // g_warning("Received rtcp");
    // ptr->rtcp_callback(src, buf, data);
}

void UDPAdaptiveStreaming::static_rtp_callback(GstElement* src, GstBuffer* buf, gpointer data)
{
    if (data != nullptr) {
        UDPAdaptiveStreaming* ptr = (UDPAdaptiveStreaming*)data;
        ptr->rtp_callback(src, buf);
    }
}

void UDPAdaptiveStreaming::rtp_callback(GstElement* src, GstBuffer* buf)
{
    guint32 buffer_size;
    buffer_size = gst_buffer_get_size(buf);
    qos_estimator.estimate_rtp_pkt_size(buffer_size);
    qos_estimator.estimate_encoding_rate(buffer_size);
}

void UDPAdaptiveStreaming::rtcp_callback(GstElement* src, GstBuffer* buf)
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
