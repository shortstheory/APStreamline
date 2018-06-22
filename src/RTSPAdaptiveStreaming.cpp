#include "RTSPAdaptiveStreaming.h"
#include <iostream>


RTSPAdaptiveStreaming::RTSPAdaptiveStreaming(string _device, CameraType type, string _uri, GstRTSPServer* server): 
                                            GenericAdaptiveStreaming(_device, type), uri(_uri), rtsp_server(server)
{
    // if (link_all_elements()) {
    //     g_warning("GOOdlink:)");
    // } else {
    //     g_warning("jail link");
    // }
    init_media_factory();
}

RTSPAdaptiveStreaming::~RTSPAdaptiveStreaming()
{
}

bool RTSPAdaptiveStreaming::link_all_elements()
{
    if (camera_type == CameraType::RAW_CAM) {
        if (!gst_element_link_many(v4l2_src, src_capsfilter, videoconvert, h264_encoder, h264_parser, rtph264_payloader, NULL)) {
            return false;
        }
    } else if (camera_type == CameraType::H264_CAM) {
        if (!gst_element_link_many(v4l2_src, src_capsfilter, h264_parser, rtph264_payloader, NULL)) {
            return false;
        }
    }
    return true;
}

void RTSPAdaptiveStreaming::init_media_factory()
{
    GstRTSPMediaFactory* media_factory;
    GstRTSPMountPoints *mounts;
    mounts = gst_rtsp_server_get_mount_points(rtsp_server);
    media_factory = gst_rtsp_media_factory_new();

    // time for some naughty business!
    // GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->_gst_reserved[0] = this;
    // GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->create_element = create_custom_pipeline;
    string launch_string = "v4l2src device=" + device + " ! video/x-raw, width=320, height=240, framerate=30/1 ! " 
                            " x264enc tune=zerolatency threads=4 bitrate=500 ! h264parse ! rtph264pay name=pay0";
    gst_rtsp_media_factory_set_launch(media_factory, launch_string.c_str());

    gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
    g_signal_connect(media_factory, "media-constructed", G_CALLBACK(static_media_constructed_callback), this);
    g_object_unref(mounts);
    // g_object_unref(media_factory);
}

GstElement* RTSPAdaptiveStreaming::create_custom_pipeline(GstRTSPMediaFactory * factory, const GstRTSPUrl  *url)
{
    g_warning("RECpipeline!");
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory)->_gst_reserved[0];    
    
    GstElement* v4l2_src;
    GstElement* src_capsfilter;
    GstElement* videoconvert;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;

    v4l2_src = gst_element_factory_make("v4l2src", NULL);
    src_capsfilter = gst_element_factory_make("capsfilter", NULL);
    videoconvert = gst_element_factory_make("videoconvert", NULL);
    h264_parser = gst_element_factory_make("h264parse", NULL);
    h264_encoder = gst_element_factory_make("x264enc", NULL);
    rtph264_payloader = gst_element_factory_make("rtph264pay", "pay0");
    g_object_set(G_OBJECT(v4l2_src), "device", ptr->device.c_str(), NULL);
    g_object_set(G_OBJECT(h264_encoder), "tune", 0x00000004, "threads", 4, NULL);
    // gst_bin_add_many(GST_BIN(pipeline), v4l2_src, src_capsfilter, rtph264_payloader, h264_parser, NULL);
    // gst_bin_add_many(GST_BIN(pipeline), h264_encoder, videoconvert, NULL);

    return (GstElement*)ptr->pipeline;
}

void RTSPAdaptiveStreaming::static_media_constructed_callback(GstRTSPMediaFactory *media_factory, 
                                                    GstRTSPMedia *media, gpointer data)
{
    g_signal_connect(media, "prepared", G_CALLBACK(static_media_prepared_callback), data);
    g_signal_connect(media, "removed-stream", G_CALLBACK(fxn), NULL);
}

void RTSPAdaptiveStreaming::static_media_prepared_callback(GstRTSPMedia* media, gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    ptr->media_prepared_callback(media);
}

void RTSPAdaptiveStreaming::media_prepared_callback(GstRTSPMedia* media)
{
    GstElement* e = gst_rtsp_media_get_element(media);
    GstElement* parent = (GstElement*)gst_object_get_parent(GST_OBJECT(e));
    g_warning("got parent!");
    GstElement* pipeline = gst_bin_get_by_name(GST_BIN(parent), "pipeline0");
    h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), "x264enc0");
    src_capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter0");
    // GList* list = GST_BIN_CHILDREN(pipeline);
    // GList* l;
    // int i = 0;
    // for (l = list; l != NULL; l = l->next)
    // {
    // e = (GstElement*)l->data;
    // char* str = gst_element_get_name(e);
    // g_warning("element name = %s %d", str, i++);
    // }
    set_resolution(ResolutionPresets::LOW);
    rtpbin = gst_bin_get_by_name(GST_BIN(parent), "rtpbin0");
    multi_udp_sink = gst_bin_get_by_name(GST_BIN(parent), "multiudpsink0");
    add_rtpbin_probes();
}

void RTSPAdaptiveStreaming::add_rtpbin_probes()
{
    GstPad* rtcp_rr_pad;
    GstPad* rtcp_sr_pad;
    GstPad* rtp_pad;

    rtcp_rr_pad = gst_element_get_static_pad(rtpbin, "recv_rtcp_sink_0");
    rtcp_sr_pad = gst_element_get_static_pad(rtpbin, "send_rtcp_src_0");
    // rtp_pad = gst_element_get_static_pad(rtpbin, "send_rtp_src_0");
    rtp_pad = gst_element_get_static_pad(multi_udp_sink, "sink");

    gst_pad_add_probe(rtcp_rr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_pad_add_probe(rtcp_sr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_pad_add_probe(rtp_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtp_callback, this, NULL);

    g_object_unref(rtcp_rr_pad);
    g_object_unref(rtcp_sr_pad);
    g_object_unref(rtp_pad);
    // GList* pads = GST_ELEMENT_PADS(rtpbin);
    // GstPad* p;
    // GList* l;

    // for (l = pads; l != NULL; l = l->next)
    // {
    //     p = (GstPad*)l->data;
    //     char* str = gst_pad_get_name(p);
    //     g_warning("rtpbinpad name = %s", str);
    //     // if ()
    // }
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_rtcp_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    return ptr->rtcp_callback(pad, info);
}

GstPadProbeReturn RTSPAdaptiveStreaming::rtcp_callback(GstPad* pad, GstPadProbeInfo* info)
{
    g_warning("H264 rate - %d", h264_bitrate);
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {    
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
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_rtp_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    return ptr->rtp_callback(pad, info);
}

GstPadProbeReturn RTSPAdaptiveStreaming::rtp_callback(GstPad* pad, GstPadProbeInfo* info)
{
    guint32 buffer_size;
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        buffer_size = gst_buffer_get_size(buf);
        if (buffer_size == 14) {
            buffer_size = 1442;
        }
        // g_warning("BUFFERSIZE %d", buffer_size);
        qos_estimator.estimate_rtp_pkt_size(buffer_size);
        qos_estimator.estimate_encoding_rate(buffer_size);
    }
    return GST_PAD_PROBE_OK;
}