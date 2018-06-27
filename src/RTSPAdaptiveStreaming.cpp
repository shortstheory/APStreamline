#include "RTSPAdaptiveStreaming.h"
#include <iostream>


RTSPAdaptiveStreaming::RTSPAdaptiveStreaming(string _device, CameraType type, string _uri, GstRTSPServer* server):
    GenericAdaptiveStreaming(_device, type), uri(_uri), rtsp_server(server)
{
    init_media_factory();
}

RTSPAdaptiveStreaming::~RTSPAdaptiveStreaming()
{
}

void RTSPAdaptiveStreaming::init_media_factory()
{
    GstRTSPMediaFactory* media_factory;
    GstRTSPMountPoints *mounts;
    mounts = gst_rtsp_server_get_mount_points(rtsp_server);
    media_factory = gst_rtsp_media_factory_new();

    string launch_string;

    if (camera_type == CameraType::RAW_CAM) {
        launch_string = "v4l2src device=" + device + " ! video/x-raw, width=320, height=240, framerate=30/1 ! "
                            " x264enc tune=zerolatency threads=4 bitrate=500 ! h264parse ! rtph264pay name=pay0";
    } else if (camera_type == CameraType::H264_CAM) {
        launch_string = "v4l2src device=" + device + " ! video/x-h264, width=320, height=240, framerate=30/1 ! "
                            " h264parse ! rtph264pay name=pay0";
    }
    gst_rtsp_media_factory_set_launch(media_factory, launch_string.c_str());

    gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
    g_signal_connect(media_factory, "media-constructed", G_CALLBACK(static_media_constructed_callback), this);
    g_object_unref(mounts);
}

void RTSPAdaptiveStreaming::static_media_constructed_callback(GstRTSPMediaFactory *media_factory,
        GstRTSPMedia *media, gpointer data)
{
    g_signal_connect(media, "prepared", G_CALLBACK(static_media_prepared_callback), data);
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
    GstElement* element;
    GstElement* pipeline;

    multi_udp_sink = NULL;

    string str;
    GList* list = GST_BIN_CHILDREN(parent);
    GList* l;
    for (l = list; l != NULL; l = l->next) {
        element = (GstElement*)l->data;
        str = gst_element_get_name(element);
#ifdef __amd64__
        if ((str.find("pipeline") != std::string::npos)) {
#endif
#ifdef __arm__
        if (str.find("bin") != std::string::npos)) {
#endif
            pipeline = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }
        if (str.find("rtpbin") != std::string::npos) {
            rtpbin = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }
        if (str.find("multiudpsink") != std::string::npos) {
            g_warning("Identified %s", str.c_str());
            multi_udp_sink = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }
        // g_warning("element name = %s", str.c_str());
    }

    list = GST_BIN_CHILDREN(pipeline);
    for (l = list; l != NULL; l = l->next) {
        element = (GstElement*)l->data;
        str = gst_element_get_name(element);
        // g_warning("String val - %s", str.c_str());
        if (camera_type == CameraType::RAW_CAM) {
            if (str.find("x264enc") != std::string::npos) {
                h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
            }
        }
        if (str.find("capsfilter") != std::string::npos) {
            src_capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
        }

        // there should be only 1 payloader, but just check later on
        if (str.find("pay") != std::string::npos) {
            rtph264_payloader = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
        }
    }

    set_resolution(ResolutionPresets::LOW);
    add_rtpbin_probes();
}

void RTSPAdaptiveStreaming::add_rtpbin_probes()
{
    GstPad* rtcp_rr_pad;
    GstPad* rtcp_sr_pad;
    GstPad* payloader_pad;

    rtcp_rr_pad = gst_element_get_static_pad(rtpbin, "recv_rtcp_sink_0");
    rtcp_sr_pad = gst_element_get_static_pad(rtpbin, "send_rtcp_src_0");
    payloader_pad = gst_element_get_static_pad(rtph264_payloader, "sink");

    gst_pad_add_probe(rtcp_rr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_pad_add_probe(rtcp_sr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_pad_add_probe(payloader_pad, GST_PAD_PROBE_TYPE_BUFFER, static_payloader_callback, this, NULL);

    g_object_unref(rtcp_rr_pad);
    g_object_unref(rtcp_sr_pad);
    g_object_unref(payloader_pad);
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

GstPadProbeReturn RTSPAdaptiveStreaming::static_payloader_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    return ptr->payloader_callback(pad, info);
}

GstPadProbeReturn RTSPAdaptiveStreaming::payloader_callback(GstPad* pad, GstPadProbeInfo* info)
{
    guint32 buffer_size;
    guint64 bytes_sent;
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        buffer_size = gst_buffer_get_size(buf);
        g_object_get(multi_udp_sink, "bytes-served", &bytes_sent, NULL);
        qos_estimator.calculate_bitrates(bytes_sent, buffer_size);
    }
    return GST_PAD_PROBE_OK;
}
