#ifndef CAMERA_H
#define CAMERA_H

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
    string camera_name;
    string capsfilter;
    uint32_t supported_qualities;

    bool fallback;
    bool dynamic_res;
    bool dynamic_bitrate;
    unordered_map<string, bool> encoder_params_bool;
    unordered_map<string, int> encoder_params_int;
    unordered_map<Quality::Level, pair<int,int>> resolutions;
    unordered_map<Quality::Level, int> framerates;

    int default_framerate;
    int default_res;

    guint32 steady_state_min;
    guint32 steady_state_max;
    guint32 steady_state_increment;
    guint32 steady_state_decrement;

    guint32 congested_state_min;
    guint32 congested_state_max;
    guint32 congested_state_increment;
    guint32 congested_state_decrement;

    guint32 low_bitrate;
    guint32 medium_bitrate;
    guint32 high_bitrate;

    guint32 min_bitrate;
    guint32 max_bitrate;
    guint32 increment_bitrate;
    guint32 decrement_bitrate;

    Quality current_quality;
    virtual bool read_configuration(Setting& camera_config, Setting& quality_config);

public:
    Camera(string device, Quality q);
    virtual ~Camera(){}
    virtual bool set_element_references(GstElement *pipeline) = 0;
    virtual bool set_bitrate(guint32 _bitrate);
    virtual bool set_quality(Quality q);
    Quality get_quality();
    string get_device_path();
    void set_bitrates_constants(bool congested);
    virtual string generate_capsfilter() const;
    virtual string generate_launch_string() const = 0;
    virtual void improve_quality(bool congested) = 0;
    virtual void degrade_quality(bool congested) = 0;
    bool dynamic_res_capability() const;
    bool dynamic_bitrate_capability() const;
};

#endif