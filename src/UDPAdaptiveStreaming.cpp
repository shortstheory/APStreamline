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

bool UDPAdaptiveStreaming::init_elements()
{
    pipeline = gst_pipeline_new("adaptive-pipeline");
    src_capsfilter = gst_element_factory_make("capsfilter", NULL);

    // choose encoder according to ctr next time
    h264_parser = gst_element_factory_make("h264parse", NULL);
    v4l2_src = gst_element_factory_make("v4l2src", NULL);
    text_overlay = gst_element_factory_make("textoverlay", NULL);
    if (camera_type == CameraType::RAW_CAM) {
        h264_encoder = gst_element_factory_make("x264enc", NULL);
        videoconvert = gst_element_factory_make("videoconvert", NULL);
    }
    else if (camera_type == CameraType::H264_CAM) {
        // do nothing
    }

    rtph264_payloader = gst_element_factory_make("rtph264pay", "pay0");
    // FIXME: Update this soon!!!
    if (!pipeline && !src_capsfilter && !rtph264_payloader && !h264_parser && !v4l2_src) {
        if (camera_type == CameraType::RAW_CAM && !h264_encoder) {
            return false;
        }
    }
    return true;
}

void UDPAdaptiveStreaming::init_element_properties()
{
    set_resolution(ResolutionPresets::LOW);

    g_object_set(G_OBJECT(v4l2_src), "device", device.c_str(), NULL);
    g_object_set(G_OBJECT(text_overlay), "text", device.c_str(), NULL);
    g_object_set(G_OBJECT(text_overlay), "valignment", 2, NULL); //top
    g_object_set(G_OBJECT(text_overlay), "halignment", 0, NULL); //left
    g_object_set(G_OBJECT(text_overlay), "font-desc", "Sans, 9", NULL);
    // valignment=2 halignment=0
    if (camera_type == CameraType::RAW_CAM) {
        g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "threads", 4, NULL);
    }
    else if (camera_type == CameraType::H264_CAM) {
        // g_object_set(G_OBJECT(H264_CAM_src), "bitrate", 1000000, NULL);
    }
}

bool UDPAdaptiveStreaming::init_rtp_elements()
{
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!rtpbin && !rr_rtcp_identity && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink &&
        !rtcp_udp_sink) {
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
    gst_bin_add_many(GST_BIN(pipeline), rtpbin, rr_rtcp_identity,
                     sr_rtcp_identity, video_udp_sink, rtcp_udp_sink, rtcp_udp_src, NULL);
}



bool UDPAdaptiveStreaming::link_all_elements()
{
    // first link all the elements with autoplugging
    if (camera_type == CameraType::RAW_CAM) {
        if (!(gst_element_link_many(v4l2_src, src_capsfilter, text_overlay, videoconvert, h264_encoder, h264_parser, rtph264_payloader, NULL) &&
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
        gst_element_link_many(rtpbin, video_udp_sink, NULL);
        g_signal_connect(rr_rtcp_identity, "handoff", G_CALLBACK(static_callback), this);
        g_signal_connect(sr_rtcp_identity, "handoff", G_CALLBACK(static_callback), this);
        gst_pad_add_probe(gst_element_get_static_pad(rtph264_payloader,"sink"), GST_PAD_PROBE_TYPE_BUFFER, static_payloader_callback, this, NULL);

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
    // while (more) {
        qos_estimator.handle_rtcp_packet(packet);
        adapt_stream();
        // more = gst_rtcp_packet_move_to_next(packet);
    // }
    free(rtcp_buffer);
    free(packet);
}

GstPadProbeReturn UDPAdaptiveStreaming::static_payloader_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    UDPAdaptiveStreaming* ptr = (UDPAdaptiveStreaming*)data;
    return ptr->payloader_callback(pad, info);
}

GstPadProbeReturn UDPAdaptiveStreaming::payloader_callback(GstPad* pad, GstPadProbeInfo* info)
{
    guint32 buffer_size;
    guint64 bytes_sent;
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        buffer_size = gst_buffer_get_size(buf);
        g_object_get(video_udp_sink, "bytes-served", &bytes_sent, NULL);
        qos_estimator.calculate_bitrates(bytes_sent, buffer_size);
    }
    return GST_PAD_PROBE_OK;
}

