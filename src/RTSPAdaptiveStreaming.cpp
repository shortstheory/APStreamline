#include "RTSPAdaptiveStreaming.h"
#include <iostream>


RTSPAdaptiveStreaming::RTSPAdaptiveStreaming(string _device, CameraType type, string _uri, GstRTSPServer* server): 
                                            GenericAdaptiveStreaming(_device, type), uri(_uri), rtsp_server(server)
{
    if (link_all_elements()) {
        g_warning("GOOdlink:)");
    } else {
        g_warning("jail link");
    }
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
    GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->_gst_reserved[0] = this;
    GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->create_element = create_custom_pipeline;
    gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
    g_signal_connect(media_factory, "media-constructed", G_CALLBACK(static_media_constructed_callback), this);
    g_object_unref(mounts);
    // g_object_unref(media_factory);
}

GstElement* RTSPAdaptiveStreaming::create_custom_pipeline(GstRTSPMediaFactory * factory, const GstRTSPUrl  *url)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory)->_gst_reserved[0];    
    return (GstElement*)ptr->pipeline;
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
    rtpbin = gst_bin_get_by_name(GST_BIN(parent), "rtpbin0");
    add_rtpbin_probes();
}

void RTSPAdaptiveStreaming::add_rtpbin_probes()
{
    GstPad* rtcp_rr_pad;
    GstPad* rtcp_sr_pad;
    GstPad* rtp_pad;

    rtcp_rr_pad = gst_element_get_static_pad(rtpbin, "recv_rtcp_sink_0");
    rtcp_sr_pad = gst_element_get_static_pad(rtpbin, "send_rtcp_src_0");
    rtp_pad = gst_element_get_static_pad(rtpbin, "send_rtp_sink_0");

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