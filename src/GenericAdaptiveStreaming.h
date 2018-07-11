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
#include "Constants.h"

using namespace std;

class GenericAdaptiveStreaming {
private:
    guint32 MIN_BITRATE;
    guint32 MAX_BITRATE;
    guint32 DEC_BITRATE;
    guint32 INC_BITRATE;

    enum NetworkState {STEADY, CONGESTION} network_state;
    guint32 successive_transmissions;

    string video_caps_string;

    // better off as a char array, change it later
    string video_presets[3];
    guint32 bitrate_presets[3];

    void set_encoding_bitrate(guint32 bitrate);
    void set_state_constants();

    void increase_resolution();
    void decrease_resolution();

    void improve_quality();
    void degrade_quality();

public:
    int current_quality;
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

    void pipeline_add_elements();

    void change_quality_preset(int quality);
    void set_resolution(ResolutionPresets setting);
    void adapt_stream();
    bool play_pipeline();
    bool pause_pipeline();
    bool change_source(string _device);
    GstBus* get_pipeline_bus();
};

#endif