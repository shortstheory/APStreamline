#include "Camera.h"
#include <gst/gst.h>

class MJPGCamera : public Camera
{
protected:
    GstElement* encoder;
    GstElement* capsfilter;
    string encoder_name;
    guint32 bitrate;
public:
    MJPGCamera(string device, Quality q) : Camera(device, q)
    {
    }
    virtual bool set_element_references(GstElement* pipeline) override
    {
        encoder = gst_bin_get_by_name(GST_BIN(pipeline), encoder_name.c_str());
        capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    }
    virtual bool set_bitrate(guint32 _bitrate) override
    {
        if (_bitrate < min_bitrate) {
            bitrate = min_bitrate;
        } else if (_bitrate > max_bitrate) {
            bitrate = max_bitrate;
        } else {
            bitrate = _bitrate;
        }
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
    // call from ctr
    // set min bitrate in ctr
    // and quality
    // camera_name
    virtual bool read_configuration(Setting &config) override
    {
        Camera::read_configuration(config);
        // change this here
        encoder_name = static_cast<const char *>(config.lookup("encoder_name"));
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
    virtual void improve_quality(bool congested) override
    {
        set_bitrates_constants(congested);
        set_bitrate(bitrate+increment_bitrate);
        if (current_quality.get_quality_level() == Quality::QualityLevel::LOW && bitrate > medium_bitrate) {
            set_quality(Quality::get_quality(Quality::QualityLevel::MEDIUM));
        } else if (current_quality.get_quality_level() == Quality::QualityLevel::MEDIUM && bitrate > high_bitrate) {
            set_quality(Quality::get_quality(Quality::QualityLevel::HIGH));
        }
    }
    virtual void degrade_quality(bool congested) override
    {
        set_bitrates_constants(congested);
        set_bitrate(bitrate-decrement_bitrate);
        if (current_quality.get_quality_level() == Quality::QualityLevel::MEDIUM && bitrate < medium_bitrate) {
            set_quality(Quality::get_quality(Quality::QualityLevel::LOW));
        } else if (current_quality.get_quality_level() == Quality::QualityLevel::HIGH && bitrate < high_bitrate) {
            set_quality(Quality::get_quality(Quality::QualityLevel::MEDIUM));
        }
    }
};