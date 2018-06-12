#include "AdaptiveStreaming.h" 
#include <functional>

AdaptiveStreaming::AdaptiveStreaming(string _device, string _ip_addr, CameraType type) :
                                     device(_device), receiver_ip_addr(_ip_addr), camera_type(type)
{
    video_presets.push_back("video/x-raw, width=(int)320, height=(int)240, framerate=(fraction)30/1");
    video_presets.push_back("video/x-raw, width=(int)640, height=(int)480, framerate=(fraction)30/1");
    video_presets.push_back("video/x-raw, width=(int)1280, height=(int)720, framerate=(fraction)30/1");

    bitrate_presets[ResolutionPresets::LOW] = 500;
    bitrate_presets[ResolutionPresets::MED] = 1500;
    bitrate_presets[ResolutionPresets::HIGH] = 3500;

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
    src_capsfilter = gst_element_factory_make("capsfilter", NULL);
    // choose encoder according to ctr next time
    if (camera_type == CameraType::V4L2CAM) {
        v4l2_src = gst_element_factory_make("v4l2src", NULL);
        h264_encoder = gst_element_factory_make("x264enc", NULL);
        h264_parser = gst_element_factory_make("h264parse", NULL);
    } else if (camera_type == CameraType::RPICAM) {
        rpicam_src = gst_element_factory_make("rpicamsrc", NULL);
    }

    rtph264_payloader = gst_element_factory_make("rtph264pay", NULL);
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rtp_identity = gst_element_factory_make("identity", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!pipeline && !src_capsfilter && !rtph264_payloader && !rtpbin && !rr_rtcp_identity
        && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink && !rtcp_udp_sink && !rtp_identity) {
        if (camera_type == CameraType::V4L2CAM && !v4l2_src && !h264_encoder && !h264_parser) {
            return false;
        } else if (camera_type == CameraType::RPICAM && !rpicam_src) {
            return false;
        }
    }
    return true;
}

void AdaptiveStreaming::init_element_properties()
{
    set_resolution(ResolutionPresets::LOW);

    if (camera_type == CameraType::V4L2CAM) {
        g_object_set(G_OBJECT(v4l2_src), "device", device.c_str(), NULL);
        g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "threads", 4, NULL);
    } else if (camera_type == CameraType::RPICAM) {
        g_object_set(G_OBJECT(rpicam_src), "bitrate", 1000000, "video-direction", 180, NULL);
    }
    g_object_set(G_OBJECT(rtcp_udp_src), "caps", gst_caps_from_string("application/x-rtcp"), 
                        "port", rtcp_src_port, NULL);
    g_object_set(G_OBJECT(rtpbin), "latency", 0, NULL);
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
    if (qos_report.get_fraction_lost() == 0) {
        if (qos_report.get_encoding_bitrate() < qos_report.get_estimated_bitrate() * 1.5) {
            improve_quality();
        } else {
            degrade_quality();
        }
    } else {
        decrease_resolution();
    }
}

void AdaptiveStreaming::improve_quality()
{
    set_encoding_bitrate(h264_bitrate+bitrate_inc);
    if (current_res == ResolutionPresets::LOW && 
        h264_bitrate > bitrate_presets[ResolutionPresets::MED]) {
            set_resolution(ResolutionPresets::MED);
    } else if (current_res == ResolutionPresets::MED && 
        h264_bitrate > bitrate_presets[ResolutionPresets::HIGH]) {
            set_resolution(ResolutionPresets::HIGH);
    }
}

void AdaptiveStreaming::degrade_quality()
{
    set_encoding_bitrate(h264_bitrate-bitrate_dec);
    if (current_res == ResolutionPresets::HIGH && 
        h264_bitrate < bitrate_presets[ResolutionPresets::MED]) {
            set_resolution(ResolutionPresets::MED);
    } else if (current_res == ResolutionPresets::MED && 
        h264_bitrate < bitrate_presets[ResolutionPresets::LOW]) {
            set_resolution(ResolutionPresets::LOW);
    }
}

void AdaptiveStreaming::set_encoding_bitrate(guint32 bitrate)
{
    if (bitrate >= min_bitrate && bitrate <= max_bitrate) {
        h264_bitrate = bitrate;
        g_object_set(G_OBJECT(h264_encoder), "bitrate", bitrate, NULL);
    }
}

// presets
// LOW - 500kbps
// MED - 1500kbps
// HIGH - 3500kbps

void AdaptiveStreaming::set_resolution(ResolutionPresets setting)
{
    g_warning("RES CHANGE! %d %ul", setting, h264_bitrate);

    string caps_filter_string;
    caps_filter_string = video_presets[setting];
    set_encoding_bitrate(bitrate_presets[setting]);
    current_res = setting;
    GstCaps* src_caps;
    src_caps = gst_caps_from_string(caps_filter_string.c_str());
    g_object_set(G_OBJECT(src_capsfilter), "caps", src_caps, NULL);
    gst_caps_unref(src_caps);
}

void AdaptiveStreaming::increase_resolution()
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

void AdaptiveStreaming::decrease_resolution()
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