#include <utility>
#include <string>
#include <unordered_map>
#include <gst/gst.h>
#include "Constants.h"
#include <libconfig.h++>
#include <regex>

using namespace std;
using namespace libconfig;

class Camera
{
private:
    string device_path;
    string launch_string;
    string capsfilter;
    bool fallback;
    bool dynamic_res;
    bool dynamic_bitrate;
    unordered_map<string, bool> encoder_params_bool;
    unordered_map<string, int> encoder_params_int;
    enum QualityProperty {WIDTH, HEIGHT, FRAMERATE};
    typedef tuple<int,int,int> Quality;
    Quality low;
    Quality med;
    Quality high;
    int default_framerate;
    int default_res;
public:
    virtual bool set_element_references(GstElement *pipeline) = 0;
    virtual void set_resolution(ResolutionPresets setting) {}
    virtual void set_bitrate(int bitrate) {}
    virtual void set_quality(int quality) {}
    virtual bool read_configuration(Setting &camera_config)
    {
        launch_string = camera_config.lookup("properties.launch_string");
        capsfilter = camera_config.lookup("properties.capsfilter");
        fallback = camera_config.lookup("properties.fallback");
        dynamic_res = camera_config.lookup("properties.dynamic_res");
        dynamic_bitrate = camera_config.lookup("properties.dynamic_bitrate");
        default_framerate = camera_config.lookup("properties.default_framerate");
        default_res = 1;

        for (int i = 0; i < camera_config["encoder_params"].getLength(); i++) {
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
    virtual string generate_capsfilter(int width, int height, int framerate) const
    {
        regex w("%width");
        regex h("%height");
        regex f("%framerate");
        string caps;
        caps = capsfilter;
        regex_replace(caps, w, to_string(width));
        regex_replace(caps, h, to_string(height));
        regex_replace(caps, f, to_string(framerate));
        return caps;
    }
};