#include <utility>
#include <string>
#include <unordered_map>
#include <gst/gst.h>
#include "Quality.h"
#include <libconfig.h++>
#include <iostream>
#include <regex>

using namespace std;
using namespace libconfig;

class Camera
{
protected:
    string device_path;
    string launch_string;
    string capsfilter;
    bool fallback;
    bool dynamic_res;
    bool dynamic_bitrate;
    unordered_map<string, bool> encoder_params_bool;
    unordered_map<string, int> encoder_params_int;

    int default_framerate;
    int default_res;

public:
    Camera()
    {
    }
    virtual bool set_element_references(GstElement* pipeline) = 0;
    virtual bool set_bitrate(int bitrate)
    {
        return dynamic_bitrate;
    }
    virtual bool set_quality(Quality q)
    {
        return dynamic_res;
    }
    virtual bool read_configuration(Setting &camera_config)
    {
        launch_string = static_cast<const char *>(camera_config.lookup("properties.launch_string"));
        capsfilter = static_cast<const char *>(camera_config.lookup("properties.capsfilter"));
        fallback = camera_config.lookup("properties.fallback");
        dynamic_res = camera_config.lookup("properties.dynamic_res");
        dynamic_bitrate = camera_config.lookup("properties.dynamic_bitrate");
        default_framerate = camera_config.lookup("properties.default_framerate");

        for (int i = 0; i < camera_config["encoder_params"].getLength(); i++)
        {
            string key;
            Setting::Type type;
            key = camera_config["encoder_params"][i].getName();
            type = camera_config["encoder_params"][i].getType();
            switch (type)
            {
            case Setting::Type::TypeBoolean:
                encoder_params_bool[key] = camera_config["encoder_params"].lookup(key);
                break;
            case Setting::Type::TypeInt:
                encoder_params_int[key] = camera_config["encoder_params"].lookup(key);
                break;
            default:
                cerr << "Other types not implement yet" << endl;
                return false;
            }
        }
        return true;
    }
    virtual string generate_capsfilter(Quality q) const
    {
        regex w("%width");
        regex h("%height");
        regex f("%framerate");
        string caps;
        caps = capsfilter;
        regex_replace(caps, w, to_string(q.getWidth()));
        regex_replace(caps, h, to_string(q.getHeight()));
        regex_replace(caps, f, to_string(q.getFramerate()));
        return caps;
    }
    virtual string generate_launch_string(Quality q, int bitrate) const = 0;
    virtual void improve_quality(bool congested) = 0;
    virtual void degrade_quality(bool congested) = 0;
    bool dynamic_res_capability() const
    {
        return dynamic_res;
    }
    bool dynamic_bitrate_capability() const
    {
        return dynamic_bitrate;
    }
};