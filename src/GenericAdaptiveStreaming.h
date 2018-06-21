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

using namespace std;

class GenericAdaptiveStreaming {
private:
    enum ResolutionPresets {LOW, MED, HIGH} current_res;

    const int bitrate_inc = 250;
    const int bitrate_dec = 1000;

    static const int max_bitrate = 8000;
    static const int min_bitrate = 50;

    string device;

    guint32 h264_bitrate;

    gint v4l2_cam_fd;


    string video_caps_string;


    // better off as a char array, change it later
    string video_presets[3];
    guint32 bitrate_presets[3];


    bool init_elements();
    void init_element_properties();
    void pipeline_add_elements();

    void set_encoding_bitrate(guint32 bitrate);
    void set_resolution(ResolutionPresets setting);
    void increase_resolution();
    void decrease_resolution();

    void improve_quality();
    void degrade_quality();

public:
    GstElement* pipeline;
    GstElement* v4l2_src;
    GstElement* src_capsfilter;
    GstElement* videoconvert;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;

    QoSEstimator qos_estimator;
    enum CameraType {RAW_CAM, H264_CAM};
    const CameraType camera_type;

    GenericAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM);
    ~GenericAdaptiveStreaming();

    void adapt_stream();
    virtual bool link_all_elements() = 0;
    bool play_pipeline();
    bool pause_pipeline();
    bool change_source(string _device);
    GstBus* get_pipeline_bus();
};

#endif