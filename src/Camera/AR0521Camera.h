#ifndef AR0521_CAMERA_H
#define AR0521_CAMERA_H

#include "Camera.h"
#include <gst/gst.h>

class AR0521Camera : public Camera
{
protected:
    GstElement *encoder;
    GstElement *capsfilter_element;

public:
    AR0521Camera(string device, Quality q);
    virtual bool set_element_references(GstElement *pipeline) override;
    virtual bool set_bitrate(guint32 _bitrate) override;
    virtual bool set_quality(Quality q) override;
    virtual string generate_launch_string() const override;
    virtual string generate_capsfilter() const override;
};

#endif