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

using namespace std;

// Generic implementation of an Adaptive Video Streamer. Contains all the necessary
// GstElements which need to be manually referenced by subclassing it.
// All the adapatation logic is handled here

class PipelineManager
{
private:
    guint32 MIN_BITRATE;
    guint32 MAX_BITRATE;
    guint32 DEC_BITRATE;
    guint32 INC_BITRATE;

    GstElement *camera;
    GstElement *src_capsfilter;
    GstElement *videoconvert;
    GstElement *h264_encoder;
    GstElement *h264_parser;
    GstElement *text_overlay;

    enum NetworkState {STEADY, CONGESTION} network_state;
    guint32 successive_transmissions;

    string video_caps_string;

    string video_presets[3];
    guint32 bitrate_presets[3];

    void set_encoding_bitrate(guint32 bitrate);
    void set_state_constants();

    void improve_quality();
    void degrade_quality();
    string device;
    int quality;
    const CameraType camera_type;

  public:
    ResolutionPresets current_res;
    guint32 h264_bitrate;

    GstElement *pipeline;
    GstElement *multi_udp_sink;
    GstElement *rtph264_payloader;
    GstElement *tee;

    QoSEstimator qos_estimator;

    PipelineManager(string _device = "/dev/video0", int quality = AUTO_PRESET, CameraType type = CameraType::MJPG_CAM);

    static int get_quality_bitrate(int quality);
    bool record_stream(bool _record_stream);
    void set_resolution(ResolutionPresets setting);
    void adapt_stream();
    int get_quality();
    void set_quality(int quality);
    string get_device();
    CameraType get_camera_type();

    // Takes the pipeline created by the launch string and iterates through it to
    // find the elements for configuration
    bool get_element_references();
};

#endif