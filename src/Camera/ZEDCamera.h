#ifndef ZED_CAMERA_H
#define ZED_CAMERA_H

#include "Camera.h"
#include <gst/gst.h>

class ZEDCamera : public Camera
{
protected:
    GstElement *encoder;
    GstElement *capsfilter;
    string encoder_name;
    virtual bool read_configuration(Setting &camera_config, Setting &quality_config) override;

public:
    ZEDCamera(string device, Quality q);
    virtual bool set_element_references(GstElement *pipeline) override;
    virtual bool set_bitrate(guint32 _bitrate) override;
    virtual bool set_quality(Quality q) override;
    virtual string generate_launch_string() const override;
};

#endif