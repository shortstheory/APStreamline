#ifndef ADAPTIVE_STREAMING_H
#define ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "QoSEstimator.h"
#include "DeviceDatatypes.h"
#include "Constants.h"
#include "FileRecorder.h"

using namespace std;

// Generic implementation of an Adaptive Video Streamer. Contains all the necessary
// GstElements which need to be manually referenced by subclassing it.
// All the adapatation logic is handled here

class GenericAdaptiveStreaming
{
private:
    guint32 MIN_BITRATE;
    guint32 MAX_BITRATE;
    guint32 DEC_BITRATE;
    guint32 INC_BITRATE;

    enum NetworkState {STEADY, CONGESTION} network_state;
    guint32 successive_transmissions;

    string video_caps_string;

    string video_presets[3];
    guint32 bitrate_presets[3];

    void set_encoding_bitrate(guint32 bitrate);
    void set_state_constants();

    void improve_quality();
    void degrade_quality();

public:
    string device;
    const CameraType camera_type;

    int current_quality;
    ResolutionPresets current_res;
    guint32 h264_bitrate;
    FileRecorder file_recorder;

    GstElement* pipeline;
    GstElement* camera;
    GstElement* src_capsfilter;
    GstElement* videoconvert;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;
    GstElement* text_overlay;
    GstElement* tee;
    GstElement* multi_udp_sink;

    QoSEstimator qos_estimator;

    GenericAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM);

    virtual ~GenericAdaptiveStreaming();
    void change_quality_preset(int quality);
    bool record_stream(bool _record_stream);
    void set_resolution(ResolutionPresets setting);
    void adapt_stream();
};

#endif