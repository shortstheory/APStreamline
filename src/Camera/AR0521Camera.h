#ifndef AR0521_CAMERA_H
#define AR0521_CAMERA_H

#include "Camera.h"
#include <gst/gst.h>

class AR0521Camera : public Camera
{
protected:
    GstElement *device;
    GstElement *capsfilter;
    guint32 bitrate;
    virtual bool read_configuration(Setting &camera_config, Setting &quality_config) override;

public:
    AR0521Camera(string device, Quality q);
    virtual bool set_element_references(GstElement *pipeline) override;
    virtual bool set_bitrate(guint32 _bitrate) override;
    virtual bool set_quality(Quality q) override;
    virtual string generate_launch_string() const override;
};

#endif