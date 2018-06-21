#ifndef CUSTOM_RTSP_MEDIA_FACTORY_H
#define CUSTOM_RTSP_MEDIA_FACTORY_H

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>

G_BEGIN_DECLS

#define CUSTOM_RTSP_MEDIA_FACTORY (custom_rtsp_media_factory_get_type())
G_DECLARE_FINAL_TYPE(CustomRTSPMediaFactory, custom_rtsp_media_factory, CUSTOM_RTSP_MEDIA, FACTORY, GstRTSPMediaFactory)

CustomRTSPMediaFactory* custom_rtsp_media_factory_new(void);

G_END_DECLS

#endif