#include "AdaptiveStreaming.h" 
#include <functional>

const string AdaptiveStreaming::receiver_ip_addr = "127.0.0.1";

AdaptiveStreaming::AdaptiveStreaming()
{
    h264_bitrate = 2000;
    init_elements();
    init_caps(1280, 720, 30);
    init_element_properties();
    pipeline_add_elements();
    if(link_all_elements()) {
        g_warning("goodlink");
    } else {
        g_warning("bad link");
    }
}

//unreffing pointers if not null can be dangerous, check this
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
    // choose encoder according to ctr next time
    h264_encoder = gst_element_factory_make("x264enc", NULL);
    h264_parser = gst_element_factory_make("h264parse", NULL);
    rtph264_payloader = gst_element_factory_make("rtph264pay", NULL);
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!pipeline && !v4l2_src && !h264_encoder && !h264_parser && !rtph264_payloader && !rtpbin && !rr_rtcp_identity
        && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink && !rtcp_udp_sink) {
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
    return (video_caps != NULL);
}

void AdaptiveStreaming::init_element_properties()
{
    g_object_set(G_OBJECT(v4l2_src), "device", "/dev/video0", NULL);
    g_object_set(G_OBJECT(rtcp_udp_src), "caps", gst_caps_from_string("application/x-rtcp"), 
                        "port", rtcp_src_port, NULL);
    g_object_set(G_OBJECT(rtpbin), "latency", 0, NULL);
    g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "bitrate", h264_bitrate, NULL);
    g_object_set(G_OBJECT(video_udp_sink), "host", receiver_ip_addr.c_str(), 
                        "port", video_sink_port, NULL);
    g_object_set(G_OBJECT(rtcp_udp_sink), "host", receiver_ip_addr.c_str(), 
                        "port", rtcp_sink_port, NULL);
}

void AdaptiveStreaming::pipeline_add_elements()
{
    gst_bin_add_many(GST_BIN(pipeline), v4l2_src, h264_encoder, h264_parser, rtph264_payloader,
                    rtpbin, rr_rtcp_identity, sr_rtcp_identity, video_udp_sink,
                    rtcp_udp_sink, rtcp_udp_src, NULL);
}

bool AdaptiveStreaming::link_all_elements()
{
    // first link all the elements with autoplugging
    if (!(gst_element_link_filtered(v4l2_src, h264_encoder, video_caps) && 
        gst_element_link(h264_encoder, h264_parser) && gst_element_link(h264_parser, rtph264_payloader) &&
        gst_element_link(rtcp_udp_src, rr_rtcp_identity))) {
        return false;
    }
    if(!gst_pad_link(gst_element_get_static_pad(rr_rtcp_identity,"src"), gst_element_get_request_pad(rtpbin, "recv_rtcp_sink_%u"))
        && !gst_pad_link(gst_element_get_static_pad(rtph264_payloader,"src"), gst_element_get_request_pad(rtpbin, "send_rtp_sink_%u"))
        && !gst_pad_link(gst_element_get_request_pad(rtpbin, "send_rtcp_src_%u"), gst_element_get_static_pad(sr_rtcp_identity,"sink"))
        && !gst_pad_link(gst_element_get_static_pad(sr_rtcp_identity,"src"), gst_element_get_static_pad(rtcp_udp_sink,"sink"))) {
        gst_element_link(rtpbin, video_udp_sink);
        auto bound_fxn = bind(callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        g_signal_connect (rr_rtcp_identity, "handoff", G_CALLBACK(&bound_fxn), NULL);
        g_signal_connect (sr_rtcp_identity, "handoff", G_CALLBACK(&bound_fxn), NULL);

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

void AdaptiveStreaming::callback(AdaptiveStreaming* ptr, GstElement *src, GstBuffer *buf, gpointer data)
{
    ptr->rtcp_callback(src, buf, data);
}

void AdaptiveStreaming::rtcp_callback(GstElement *src, GstBuffer *buf, gpointer data)
{
    g_warning("bitrate %d", h264_bitrate);
    // g_warning("rtcp_received %d", gst_rtcp_buffer_get_packet_count(buf));
    // GstRTCPBuffer *rtcpbuf = (GstRTCPBuffer*)malloc(sizeof(GstRTCPBuffer));
    // rtcpbuf->buffer = NULL;
    // // rtcpbuf->map = GST_MAP_INFO_INIT;
    // gst_rtcp_buffer_map(buf, GST_MAP_READ, rtcpbuf);
    // GstRTCPPacket *packet = (GstRTCPPacket*)malloc(sizeof(GstRTCPPacket));
    // gboolean more = gst_rtcp_buffer_get_first_packet(rtcpbuf, packet);
	// while (more) {
	// 	GstRTCPType type;
	// 	type = gst_rtcp_packet_get_type(packet);
	// 	switch (type) {
	// 	case GST_RTCP_TYPE_RR:
    //         process_rtcp_packet(packet);
    //         // if (switchvalue) {
    //         //     g_warning("Inc bitrate");
    //         //     g_object_set(G_OBJECT(enc), "bitrate", bitrate, NULL);
    //         //     switchvalue = FALSE;
    //         // } else {
    //         //     g_warning("dec bitrate");
    //         //     g_object_set(G_OBJECT(enc), "bitrate", bitrate2, NULL);
    //         //     switchvalue = TRUE;
    //         // }
    //         // send_event_to_encoder(venc, &rtcp_pkt);
    //         break;
    //     case GST_RTCP_TYPE_SR:
    //         process_sender_packet(packet);
	// 		break;
	// 	default:
	// 		g_debug("Other types");
	// 		break;
	// 	}
	// 	more = gst_rtcp_packet_move_to_next(packet);
	// }
}