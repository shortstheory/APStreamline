#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#include "RTSPAdaptiveStreaming.h"

RTSPAdaptiveStreaming::RTSPAdaptiveStreaming(string _device,
        CameraType type,
        string _uri,
        GstRTSPServer* server,
        int quality):
    GenericAdaptiveStreaming(_device, type),
    uri(_uri),
    rtsp_server(server),
    media_prepared(false)
{
    current_quality = quality;
    init_media_factory();
}

RTSPAdaptiveStreaming::~RTSPAdaptiveStreaming()
{
    gst_element_set_state(multi_udp_sink, GST_STATE_NULL);
    gst_object_unref(multi_udp_sink);
    gst_element_set_state(rtpbin, GST_STATE_NULL);
    gst_object_unref(rtpbin);
    gst_object_unref(rtsp_server);
}

void RTSPAdaptiveStreaming::init_media_factory()
{
    GstRTSPMediaFactory* media_factory;
    GstRTSPMountPoints* mounts;
    mounts = gst_rtsp_server_get_mount_points(rtsp_server);
    media_factory = gst_rtsp_media_factory_new();

    string launch_string;

    switch (camera_type) {
    case RAW_CAM:
        launch_string = "v4l2src device=" + device +
                        " ! video/x-raw, width=320, height=240, framerate=30/1"
                        " ! videoconvert"
                        " ! textoverlay"
                        " ! x264enc tune=zerolatency threads=4 bitrate=500"
                        " ! tee name=tee_element tee_element."
                        " ! queue"
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    case UVC_CAM:
        launch_string = "uvch264src device=" + device +
                        " ! name=src auto-start=true src.vidsrc "
                        " ! queue"
                        " ! video/x-h264, width=320, height=240, framerate=30/1"
                        " ! tee name=tee_element tee_element."
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    case H264_CAM:
        launch_string = "v4l2src device=" + device +
                        " ! video/x-h264, width=320, height=240, framerate=30/1"
                        " ! tee name=tee_element tee_element."
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    };
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

    multi_udp_sink = nullptr;

    string str;
    GList* list = GST_BIN_CHILDREN(parent);
    GList* l;

    for (l = list; l != nullptr; l = l->next) {
        element = (GstElement*)l->data;
        str = gst_element_get_name(element);
        if (AMD64) {
            if (str.find("pipeline") != std::string::npos) {
                pipeline = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
            }
        }
        if (ARM) {
            if (str.find("bin") != std::string::npos) {
                pipeline = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
            }
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

    for (l = list; l != nullptr; l = l->next) {
        element = (GstElement*)l->data;
        str = gst_element_get_name(element);
        g_warning("String val - %s", str.c_str());

        switch (camera_type) {
        case RAW_CAM:
            if (str.find("x264enc") != std::string::npos) {
                h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
            }
        // the lack of a break is intentional, trust me! :P
        case H264_CAM:
            if (str.find("v4l2src") != std::string::npos) {
                v4l2_src = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
            }
            break;
        case UVC_CAM:
            if (str.find("uvch264src") != std::string::npos) {
                v4l2_src = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
            }
            break;
        };

        if (str.find("tee_element") != std::string::npos) {
            g_warning("found tee");
            tee = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
        }
        if (str.find("textoverlay") != std::string::npos) {
            text_overlay = gst_bin_get_by_name(GST_BIN(pipeline), str.c_str());
            g_object_set(G_OBJECT(text_overlay), "valignment", 2, NULL); //top
            g_object_set(G_OBJECT(text_overlay), "halignment", 0, NULL); //left
            g_object_set(G_OBJECT(text_overlay), "font-desc", "Sans, 9", NULL);
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
    media_prepared = true;
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_probe_block_callback(GstPad* pad,
        GstPadProbeInfo* info,
        gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    return ptr->probe_block_callback(pad, info);
}

GstPadProbeReturn RTSPAdaptiveStreaming::probe_block_callback(GstPad* pad, GstPadProbeInfo* info)
{
    // pad is blocked, so it's ok to unlink
    gst_pad_unlink(file_recorder.tee_file_pad, file_recorder.queue_pad);
    file_recorder.disable_recorder();
    gst_element_release_request_pad(tee, file_recorder.tee_file_pad);
    g_warning("Pad Removed");
    return GST_PAD_PROBE_REMOVE;
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

bool RTSPAdaptiveStreaming::get_media_prepared()
{
    return media_prepared;
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_rtcp_callback(GstPad* pad,
        GstPadProbeInfo* info,
        gpointer data)
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
            if (current_quality == AUTO_PRESET) {
                adapt_stream();
            }
            more = gst_rtcp_packet_move_to_next(packet);
        }
        free(rtcp_buffer);
        free(packet);
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_payloader_callback(GstPad* pad,
        GstPadProbeInfo* info,

        gpointer data)
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

// make void
bool RTSPAdaptiveStreaming::record_stream(bool _record_stream)
{
    if (_record_stream) {
        return file_recorder.init_file_recorder(pipeline, tee);
    } else {
        gst_pad_add_probe(file_recorder.tee_file_pad, GST_PAD_PROBE_TYPE_BLOCK,
                          static_probe_block_callback, this, NULL);
        return true;
    }
}
