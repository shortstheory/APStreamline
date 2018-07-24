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
    gst_object_unref(rtsp_server);
}

void RTSPAdaptiveStreaming::init_media_factory()
{
    GstRTSPMediaFactory* media_factory;
    GstRTSPMountPoints* mounts;
    mounts = gst_rtsp_server_get_mount_points(rtsp_server);
    media_factory = gst_rtsp_media_factory_new();

    string launch_string;

    // Set launch string according to the type of camera
    switch (camera_type) {
    case RAW_CAM:
        launch_string = "v4l2src name=src device=" + device +
                        " ! capsfilter name=capsfilter caps=image/jpeg,width=320,height=240,framerate=30/1"
                        " ! jpegdec"
                        " ! videoconvert"
                        " ! textoverlay name=textoverlay"
                        " ! x264enc name=x264enc tune=zerolatency threads=4 bitrate=500"
                        " ! tee name=tee_element tee_element."
                        " ! queue"
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    case UVC_CAM:
        launch_string = "uvch264src device=" + device +
                        " name=src auto-start=true src.vidsrc"
                        " ! queue"
                        " ! capsfilter name=capsfilter caps=video/x-h264,width=1280,height=720,framerate=30/1"
                        " ! tee name=tee_element tee_element."
                        " ! queue"
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    case H264_CAM:
        launch_string = "v4l2src name=src device=" + device +
                        " ! queue"
                        " ! capsfilter name=capsfilter caps=video/x-h264,width=320,height=240,framerate=30/1"
                        " ! queue"
                        " ! h264parse"
                        " ! rtph264pay name=pay0";
        break;
    };
    gst_rtsp_media_factory_set_launch(media_factory, launch_string.c_str());

    gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
    g_signal_connect(media_factory, "media-constructed", G_CALLBACK(static_media_constructed_callback), this);
    g_object_unref(mounts);
}


void RTSPAdaptiveStreaming::media_prepared_callback(GstRTSPMedia* media)
{
    GstElement* element;
    GstElement* parent;
    GList* list;
    GList* list_itr;

    element = gst_rtsp_media_get_element(media);
    parent = (GstElement*)gst_object_get_parent(GST_OBJECT(element));
    multi_udp_sink = nullptr;

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
        g_warning("element name = %s", str.c_str());

        if (str.find("bin") != std::string::npos || str.find("pipeline") != std::string::npos) {
            pipeline = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }
        if (str.find("rtpbin") != std::string::npos) {
            rtpbin = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
        }

        // Older gstreamer versions on the ARM boards setup multiudpsink in a different place
        if (minor < 14) {
            if (str.find("multiudpsink") != std::string::npos) {
                g_warning("Identified %s", str.c_str());
                multi_udp_sink = gst_bin_get_by_name(GST_BIN(parent), str.c_str());
            }
        }
    }

    if (get_element_references()) {
        g_warning("Elements referenced!");
    } else {
        g_warning("Some elements not referenced");
    }

    set_resolution(ResolutionPresets::LOW);
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
    if (element_name.find("multiudpsink") != std::string::npos && !multi_udp_sink) {
        g_warning("Identified %s", element_name.c_str());
        multi_udp_sink = element;
    }
}

// Called once the client has disconnected, allowing us to clean up the stream
void RTSPAdaptiveStreaming::media_unprepared_callback(GstRTSPMedia* media)
{
    file_recorder.disable_recorder();
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    gst_element_set_state(rtpbin, GST_STATE_NULL);
    gst_object_unref(rtpbin);
    current_quality = AUTO_PRESET;
    g_warning("Stream disconnected!");
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
    file_recorder.disable_recorder();
    g_warning("Pad Removed");
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
    // g_warning("H264 rate - %d", h264_bitrate);
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        GstRTCPBuffer *rtcp_buffer = (GstRTCPBuffer*)malloc(sizeof(GstRTCPBuffer));
        rtcp_buffer->buffer = nullptr;
        gst_rtcp_buffer_map(buf, GST_MAP_READ, rtcp_buffer);
        GstRTCPPacket *packet = (GstRTCPPacket*)malloc(sizeof(GstRTCPPacket));
        gboolean more = gst_rtcp_buffer_get_first_packet(rtcp_buffer, packet);
        //same buffer can have an SDES and an RTCP pkt
        while (more) {
            qos_estimator.handle_rtcp_packet(packet);
            if (current_quality == AUTO_PRESET && gst_rtcp_packet_get_type(packet) == GST_RTCP_TYPE_RR) {
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

// We measure the size of every buffer on the rtph264payloader to get the encoding
// bitrate estimate
GstPadProbeReturn RTSPAdaptiveStreaming::payloader_callback(GstPad* pad, GstPadProbeInfo* info)
{
    guint32 buffer_size;
    guint64 bytes_sent;
    GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf != nullptr) {
        buffer_size = gst_buffer_get_size(buf);
        if (multi_udp_sink) {
            g_object_get(multi_udp_sink, "bytes-served", &bytes_sent, NULL);
        }
        qos_estimator.calculate_bitrates(bytes_sent, buffer_size);
    }
    return GST_PAD_PROBE_OK;
}

bool RTSPAdaptiveStreaming::get_element_references()
{
    if (pipeline) {
        tee = gst_bin_get_by_name(GST_BIN(pipeline), "tee_element");
        rtph264_payloader = gst_bin_get_by_name(GST_BIN(pipeline), "pay0");
        v4l2_src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
        src_capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");

        switch (camera_type) {
        case RAW_CAM:
            h264_encoder = gst_bin_get_by_name(GST_BIN(pipeline), "x264enc");
            text_overlay = gst_bin_get_by_name(GST_BIN(pipeline), "textoverlay");
            if (tee && rtph264_payloader && v4l2_src && src_capsfilter && h264_encoder && text_overlay) {
                g_object_set(G_OBJECT(text_overlay),
                             "valignment", 2,
                             "halignment", 0,
                             "font-desc", "Sans, 9", NULL);
                g_object_set(G_OBJECT(h264_encoder), 
                             "tune", 0x00000004,
                             "threads", 4,
                             "key-int-max", I_FRAME_INTERVAL,
                             // intra-refresh breaks an iframe over multiple frames
                             "intra-refresh", TRUE,
                             NULL);
                return true;
            } else {
                return false;
            }
        case H264_CAM:
            int v4l2_cam_fd;
            if (v4l2_src) {
                g_object_get(v4l2_src, "device-fd", &v4l2_cam_fd, NULL);
                if (v4l2_cam_fd > 0) {
                    v4l2_control veritcal_flip;
                    veritcal_flip.id = V4L2_CID_VFLIP;
                    veritcal_flip.value = TRUE;

                    v4l2_control horizontal_flip;
                    horizontal_flip.id = V4L2_CID_HFLIP;
                    horizontal_flip.value = TRUE;

                    v4l2_control i_frame_interval;
                    i_frame_interval.id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
                    i_frame_interval.value = I_FRAME_INTERVAL;

                    if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &veritcal_flip) == -1 ||
                        ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &horizontal_flip) == -1 ||
                        ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &i_frame_interval) == -1) {
                            return false;
                    }
                }
            }
        case UVC_CAM:
            if (tee && rtph264_payloader && v4l2_src && src_capsfilter) {
                return true;
            } else {
                return false;
            }
        };
    }
    return false;
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

    payloader_pad = gst_element_get_static_pad(rtph264_payloader, "sink");
    gst_pad_add_probe(payloader_pad, GST_PAD_PROBE_TYPE_BUFFER, static_payloader_callback, this, NULL);
    gst_object_unref(payloader_pad);
}

bool RTSPAdaptiveStreaming::get_media_prepared()
{
    return media_prepared;
}

void RTSPAdaptiveStreaming::record_stream(bool _record_stream)
{
    if (_record_stream) {
        file_recorder.init_file_recorder(pipeline, tee);
    } else {
        if (file_recorder.tee_file_pad) {
            gst_pad_add_probe(file_recorder.tee_file_pad, GST_PAD_PROBE_TYPE_BLOCK,
                              static_probe_block_callback, this, NULL);
        } else {
            g_warning("File pad missing");
        }
    }
}

// On changing the resolution while the CC is recording, we stop the recording,
// to avoid any serious concurrency issues which can otherwise occur
void RTSPAdaptiveStreaming::set_device_properties(int quality, bool _record_stream)
{
    // We can't have the capsfilter changing when recording from the CC, so we disable it for AUTO mode
    if (camera_type != H264_CAM) {
        if (quality == AUTO_PRESET && camera_type != UVC_CAM) {
            record_stream(false);
            change_quality_preset(quality);
            return;
        }
        if (quality == current_quality) {
            record_stream(_record_stream);
        } else {
            record_stream(false);
            change_quality_preset(quality);
        }
    } else {
        record_stream(false);
        change_quality_preset(quality);
    }
}