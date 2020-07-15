#include "Camera.h"
#include <gst/gst.h>

class MJPGCamera : public Camera
{
protected:
    GstElement *encoder;
    GstElement *capsfilter;
    string encoder_name;

public:
    virtual bool set_element_references(GstElement *pipeline) override
    {
        encoder = gst_bin_get_by_name(GST_BIN(pipeline), encoder_name.c_str());
        capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    }
    virtual bool set_bitrate(int bitrate) override
    {
        g_object_set(G_OBJECT(encoder), "bitrate", bitrate, NULL);
        return true;
    }
    virtual bool set_quality(Quality q) override
    {
        string capsfilter_string;
        capsfilter_string = generate_capsfilter(q);
        GstCaps *caps;
        caps = gst_caps_from_string(capsfilter_string.c_str());
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        gst_caps_unref(caps);
        return true;
    }
    virtual bool read_configuration(Setting &camera_config) override
    {
        Camera::read_configuration(camera_config);
        encoder_name = static_cast<const char *>(camera_config.lookup("encoder_name"));
    }
    virtual string generate_launch_string(Quality q, int bitrate) const override
    {
        string capsfilter_string;
        capsfilter_string = generate_capsfilter(q);
        regex d("%device");
        regex cf("%capsfilter");
        regex enc("%encoder");
        regex br("%bitrate");
        regex_replace(launch_string, d, device_path);
        regex_replace(launch_string, cf, capsfilter_string);
        regex_replace(launch_string, enc, encoder_name);
        regex_replace(launch_string, br, to_string(bitrate));
        return launch_string;
    }
};