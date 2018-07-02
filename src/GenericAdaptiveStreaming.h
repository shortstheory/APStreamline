#ifndef ADAPTIVE_STREAMING_H
#define ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>

#include "QoSEstimator.h"
#include "DeviceDatatypes.h"

using namespace std;

class GenericAdaptiveStreaming {
private:
    const int bitrate_dec = 1000;
    static const int min_bitrate = 50;

#ifdef __amd64__
    static const int max_bitrate = 8000;
    const int bitrate_inc = 250;
#endif
#ifdef __arm__
    const int bitrate_inc = 125;
    static const int max_bitrate = 3200;
#endif

    gint v4l2_cam_fd;
    bool res_inc;

    string video_caps_string;

    // better off as a char array, change it later
    string video_presets[3];
    guint32 bitrate_presets[3];

    void set_encoding_bitrate(guint32 bitrate);
    void increase_resolution();
    void decrease_resolution();

    void improve_quality();
    void degrade_quality();

public:
    string device;
    ResolutionPresets current_res;
    guint32 h264_bitrate;
    GstElement* pipeline;
    GstElement* v4l2_src;
    GstElement* src_capsfilter;
    GstElement* videoconvert;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;
    GstElement* text_overlay;

    QoSEstimator qos_estimator;
    const CameraType camera_type;

    GenericAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM);

    virtual ~GenericAdaptiveStreaming();

    bool init_elements();
    void init_element_properties();
    void pipeline_add_elements();

    void set_resolution(ResolutionPresets setting);
    void adapt_stream();
    bool play_pipeline();
    bool pause_pipeline();
    bool change_source(string _device);
    GstBus* get_pipeline_bus();
};

#endif