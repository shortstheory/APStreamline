#ifndef CUSTOM_MEDIA_FACTORY_H
#define CUSTOM_MEDIA_FACTORY_H

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>

G_BEGIN_DECLS

#define CUSTOM_MEDIA_FACTORY (custom_media_factory_get_type ())
G_DECLARE_FINAL_TYPE(CustomMediaFactory, custom_media_factory, CUSTOM_MEDIA, FACTORY, GstRTSPMediaFactory)

CustomMediaFactory* custom_media_factory_new(void);

G_END_DECLS

#endif