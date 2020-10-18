#ifndef RPI_CAMERA_H
#define RPI_CAMERA_H

#include "Camera.h"
#include <gst/gst.h>

class RPiCamera : public Camera
{
protected:
    GstElement *src;
    GstElement *capsfilter;
    string encoder_name;
    virtual bool read_configuration(Setting &camera_config, Setting &quality_config) override;

public:
    RPiCamera(string device, Quality q);
    virtual bool set_element_references(GstElement *pipeline) override;
    virtual bool set_bitrate(guint32 _bitrate) override;
    virtual bool set_quality(Quality q) override;
    virtual string generate_launch_string() const override;
};

#endif