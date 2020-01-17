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
    uri(_uri),
    rtsp_server((GstRTSPServer*)gst_object_ref(server)),
    media_prepared(false),
    pipeline_manager(_device, quality, type)
{
    init_media_factory();
}

RTSPAdaptiveStreaming::~RTSPAdaptiveStreaming()
{
    if (media_factory) {
        gst_object_unref(media_factory);
    }
    gst_object_unref(rtsp_server);
}

void RTSPAdaptiveStreaming::init_media_factory()
{
    if (!media_prepared) {
        GstRTSPMountPoints* mounts;
        mounts = gst_rtsp_server_get_mount_points(rtsp_server);
        media_factory = gst_rtsp_media_factory_new();

        string launch_string;
        string device;
        device = pipeline_manager.get_device();

        int quality;
        quality = pipeline_manager.get_quality();

        int h264_bitrate;
        h264_bitrate = PipelineManager::get_quality_bitrate(quality);

        string resolution_caps;

        // Set launch string according to the type of camera
        switch (pipeline_manager.get_camera_type()) {
        case MJPG_CAM:
            resolution_caps = (quality == AUTO_PRESET) ? RAW_CAPS_FILTERS[VIDEO_320x240x30] : RAW_CAPS_FILTERS[quality];
            launch_string = "v4l2src name=src device=" + device + " ! capsfilter name=capsfilter caps=" + resolution_caps +
                            " ! jpegdec"
                            " ! videoconvert"
                            " ! textoverlay name=textoverlay"
                            " ! x264enc name=x264enc tune=zerolatency threads=4 bitrate=" + to_string(h264_bitrate) +
                            " ! tee name=tee_element tee_element."
                            " ! queue"
                            " ! h264parse config-interval=3"
                            " ! rtph264pay name=pay0";
            break;
        case UVC_CAM:
            resolution_caps = (quality == AUTO_PRESET) ? H264_CAPS_FILTERS[VIDEO_640x480x30] : H264_CAPS_FILTERS[quality];
            launch_string = "uvch264src device=" + device + " average-bitrate=" + to_string(h264_bitrate*1000) +
                            " name=src auto-start=true src.vidsrc"
                            " ! queue"
                            " ! capsfilter name=capsfilter caps=" + resolution_caps +
                            " ! tee name=tee_element tee_element."
                            " ! queue"
                            " ! h264parse config-interval=3"
                            " ! rtph264pay name=pay0";
            break;
        case H264_CAM:
            resolution_caps = (quality == AUTO_PRESET) ? H264_CAPS_FILTERS[VIDEO_320x240x30] : H264_CAPS_FILTERS[quality];
            launch_string = "v4l2src name=src device=" + device +
                            " ! queue"
                            " ! capsfilter name=capsfilter caps=" + resolution_caps +
                            " ! queue"
                            " ! h264parse config-interval=3"
                            " ! rtph264pay name=pay0";
            break;
        case JETSON_CAM:
            resolution_caps = (quality == AUTO_PRESET) ? JETSON_CAPS_FILTERS[VIDEO_640x480x30] : JETSON_CAPS_FILTERS[quality];
            launch_string = "nvarguscamerasrc name=src "
                            " ! capsfilter name=capsfilter caps=" + resolution_caps +
                            " ! omxh264enc name=omxh264enc control-rate=1 bitrate=" + to_string(h264_bitrate*1000) +
                            " ! capsfilter caps =\"video/x-h264,profile=baseline,stream-format=(string)byte-stream\""
                            " ! h264parse config-interval=3"
                            " ! rtph264pay name=pay0";
            break;
        };
        gst_rtsp_media_factory_set_launch(media_factory, launch_string.c_str());
        gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
        g_signal_connect(media_factory, "media-constructed", G_CALLBACK(static_media_constructed_callback), this);
        gst_object_unref(mounts);
    }
}

void RTSPAdaptiveStreaming::media_prepared_callback(GstRTSPMedia* media)
{
    GstElement* element;
    GstElement* parent;
    GList* list;
    GList* list_itr;

    element = gst_rtsp_media_get_element(media);
    parent = (GstElement*)gst_object_get_parent(GST_OBJECT(element));
    pipeline_manager.multi_udp_sink = nullptr;

    guint major;
    guint minor;
    guint micro;
    guint nano;
    gst_version(&major, &minor, &micro, &nano);

    if (minor >= 14) {
        g_signal_connect(parent, "deep-element-added", G_CALLBACK(static_deep_callback), this);
    }
    string str;
    list = GST_BIN_CHILDREN(parent);

    for (list_itr = list; list_itr != nullptr; list_itr = list_itr->next) {
        element = (GstElement*)list_itr->data;
        str = gst_element_get_name(element);

        if (str.find("bin") != std::string::npos || str.find("pipeline") != std::string::npos) {
            pipeline_manager.pipeline = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }
        if (str.find("rtpbin") != std::string::npos) {
            rtpbin = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }

        // Older gstreamer versions on the ARM boards setup multiudpsink in a different place
        if (minor < 14) {
            if (str.find("multiudpsink") != std::string::npos) {
                pipeline_manager.multi_udp_sink = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
            }
        }
    }

    if (!pipeline_manager.get_element_references()) {
        cerr << "Some GStreamer elements not referenced" << endl;
    }

    add_rtpbin_probes();
    media_prepared = true;
}

void RTSPAdaptiveStreaming::static_media_constructed_callback(GstRTSPMediaFactory *media_factory,
        GstRTSPMedia *media, gpointer data)
{
    g_signal_connect(media, "prepared", G_CALLBACK(static_media_prepared_callback), data);
    g_signal_connect(media, "unprepared", G_CALLBACK(static_media_unprepared_callback), data);
}

void RTSPAdaptiveStreaming::static_media_prepared_callback(GstRTSPMedia* media, gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    ptr->media_prepared_callback(media);
}

void RTSPAdaptiveStreaming::static_media_unprepared_callback(GstRTSPMedia* media, gpointer data)
{
    // use this as a way to clean up variables and refs I guess?!
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    ptr->media_unprepared_callback(media);
}

void RTSPAdaptiveStreaming::static_deep_callback(GstBin* bin,
        GstBin* sub_bin,
        GstElement* element,
        gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    ptr->deep_callback(bin, sub_bin, element);
}

void RTSPAdaptiveStreaming::deep_callback(GstBin* bin,
        GstBin* sub_bin,
        GstElement* element)
{
    string element_name;
    element_name = gst_element_get_name(element);
    // One udpsink takes care of RTCP packets and the other RTP
    if (element_name.find("multiudpsink") != std::string::npos && !pipeline_manager.multi_udp_sink) {
        pipeline_manager.multi_udp_sink = element;
    }
}

// Called once the client has disconnected, allowing us to clean up the stream
void RTSPAdaptiveStreaming::media_unprepared_callback(GstRTSPMedia* media)
{
    gst_element_set_state(pipeline_manager.pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline_manager.pipeline);

    gst_element_set_state(rtpbin, GST_STATE_NULL);
    gst_object_unref(rtpbin);
    media_prepared = false;
    init_media_factory();
    cout << "Stream disconnected!" << endl;
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
    return GST_PAD_PROBE_REMOVE;
}

GstPadProbeReturn RTSPAdaptiveStreaming::static_rtcp_callback(GstPad* pad,
        GstPadProbeInfo* info,
        gpointer data)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)data;
    return ptr->rtcp_callback(pad, info);
}

// On receiving an RTCP packet, we forward it to the QoSEstimator for extracting
// the data and generating the QoSReport
GstPadProbeReturn RTSPAdaptiveStreaming::rtcp_callback(GstPad* pad, GstPadProbeInfo* info)
{
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        GstRTCPBuffer *rtcp_buffer = (GstRTCPBuffer*)malloc(sizeof(GstRTCPBuffer));
        rtcp_buffer->buffer = nullptr;
        gst_rtcp_buffer_map(buf, GST_MAP_READ, rtcp_buffer);
        GstRTCPPacket *packet = (GstRTCPPacket*)malloc(sizeof(GstRTCPPacket));
        gboolean more = gst_rtcp_buffer_get_first_packet(rtcp_buffer, packet);
        //same buffer can have an SDES and an RTCP pkt
        while (more) {
            pipeline_manager.qos_estimator.handle_rtcp_packet(packet);
            if (pipeline_manager.get_quality() == AUTO_PRESET && gst_rtcp_packet_get_type(packet) == GST_RTCP_TYPE_RR) {
                pipeline_manager.adapt_stream();
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

// We measure the size of every buffer on the rtph264payloader to get the encoding
// bitrate estimate
GstPadProbeReturn RTSPAdaptiveStreaming::payloader_callback(GstPad* pad, GstPadProbeInfo* info)
{
    guint32 buffer_size;
    guint64 bytes_sent;
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        buffer_size = gst_buffer_get_size(buf);
        if (pipeline_manager.multi_udp_sink) {
            g_object_get(pipeline_manager.multi_udp_sink, "bytes-served", &bytes_sent, NULL);
        }
        pipeline_manager.qos_estimator.calculate_bitrates(bytes_sent, buffer_size);
    }
    return GST_PAD_PROBE_OK;
}

void RTSPAdaptiveStreaming::add_rtpbin_probes()
{
    GstPad* rtcp_rr_pad;
    GstPad* rtcp_sr_pad;
    GstPad* payloader_pad;

    rtcp_rr_pad = gst_element_get_static_pad(rtpbin, "recv_rtcp_sink_0");
    gst_pad_add_probe(rtcp_rr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_object_unref(rtcp_rr_pad);

    rtcp_sr_pad = gst_element_get_static_pad(rtpbin, "send_rtcp_src_0");
    gst_pad_add_probe(rtcp_sr_pad, GST_PAD_PROBE_TYPE_BUFFER, static_rtcp_callback, this, NULL);
    gst_object_unref(rtcp_sr_pad);

    payloader_pad = gst_element_get_static_pad(pipeline_manager.rtph264_payloader, "sink");
    gst_pad_add_probe(payloader_pad, GST_PAD_PROBE_TYPE_BUFFER, static_payloader_callback, this, NULL);
    gst_object_unref(payloader_pad);
}

bool RTSPAdaptiveStreaming::get_media_prepared()
{
    return media_prepared;
}


int RTSPAdaptiveStreaming::get_quality()
{
    return pipeline_manager.get_quality();
}

void RTSPAdaptiveStreaming::set_quality(int quality)
{
    if (quality != pipeline_manager.get_quality()) {
        pipeline_manager.set_quality(quality);
    }
}
