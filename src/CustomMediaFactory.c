#include "CustomMediaFactory.h"

struct CustomMediaFactoryClass
{
    GstRTSPMediaFactoryClass parent;
};

struct CustomMediaFactory
{
     GstRTSPMediaFactory parent;
};

static GstElement * custom_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
GType test_rtsp_media_factory_get_type   (void);


G_DEFINE_TYPE (TestRTSPMediaFactory, test_rtsp_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);

static void
test_rtsp_media_factory_class_init (TestRTSPMediaFactoryClass * test_klass)
{
  g_print("Makeing custom");
   GstRTSPMediaFactoryClass *klass = (GstRTSPMediaFactoryClass *) (test_klass);
   klass->create_element = custom_create_element;
}

static void
test_rtsp_media_factory_init (TestRTSPMediaFactory * media)
{
}