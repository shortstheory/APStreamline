#include "RTSPAdaptiveStreaming.h"
#include <iostream>


RTSPAdaptiveStreaming::RTSPAdaptiveStreaming(string _device, CameraType type, string _uri, GstRTSPServer* server): 
                                            GenericAdaptiveStreaming(_device, type), uri(_uri), rtsp_server(server)
{
    link_all_elements();
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
    GstRTSPMountPoints *mounts;
    mounts = gst_rtsp_server_get_mount_points(rtsp_server);
    media_factory = gst_rtsp_media_factory_new();

    // time for some naughty business!
    GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->create_element = create_custom_pipeline;
    GST_RTSP_MEDIA_FACTORY_GET_CLASS(media_factory)->_gst_reserved[0] = this;
    gst_rtsp_mount_points_add_factory(mounts, uri.c_str(), media_factory);
    g_warning("%p\n", (void *)this);
    g_object_unref(mounts);
}

GstElement* RTSPAdaptiveStreaming::create_custom_pipeline(GstRTSPMediaFactory * factory, const GstRTSPUrl  *url)
{
    RTSPAdaptiveStreaming* ptr = (RTSPAdaptiveStreaming*)GST_RTSP_MEDIA_FACTORY_GET_CLASS(factory)->_gst_reserved[0];
    g_warning("%p\n", (void *)ptr);
    if (ptr == NULL) {
        g_warning("FUCKKEKKE");
    } else {
        string x = ptr->geturi();
        g_warning("Should work %s", x.c_str());
    }
    return (GstElement*)ptr->pipeline;
}