#include "CustomRTSPMediaFactory.h"

struct CustomRTSPMediaFactoryClass
{
    GstRTSPMediaFactoryClass parent;
};

struct CustomRTSPMediaFactory
{
    GstRTSPMediaFactory parent;
};

static GstElement* custom_create_element(GstRTSPMediaFactory *factory, const GstRTSPUrl *url);
GType custom_rtsp_media_factory_get_type(void);

G_DEFINE_TYPE(CustomMediaFactory, custom_media_factory, GST_TYPE_RTSP_MEDIA_FACTORY);

static void custom_rtsp_media_factory_class_init(CustomRTSPMediaFactoryClass * test_klass)
{
    GstRTSPMediaFactoryClass *klass = (GstRTSPMediaFactoryClass*) (test_klass);
    klass->create_element = custom_create_element;
}

static void custom_rtsp_media_factory_init (TestRTSPMediaFactory * media)
{
}