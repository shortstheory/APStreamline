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
#include "../Camera/Camera.h"
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
    shared_ptr<Camera> cam;

    // GstElement* camera;
    // GstElement* src_capsfilter;
    // GstElement* videoconvert;
    // GstElement* h264_encoder;
    // GstElement* text_overlay;

    // enum NetworkState {STEADY, CONGESTION} network_state;
    bool congested;
    guint32 successive_transmissions;

    // string video_caps_string;

    // string video_presets[3];

    // string device;
    // int quality;
    const CameraType camera_type;
    bool auto_mode;

  public:
    ResolutionPresets current_res;
    guint32 h264_bitrate;

    GstElement* pipeline;
    GstElement* multi_udp_sink;
    GstElement* rtph264_payloader;
    GstElement* tee;

    QoSEstimator qos_estimator;

    PipelineManager(string _device = "/dev/video0", CameraType type = CameraType::MJPG_CAM);

    void adapt_stream();
    // int get_quality();
    // void set_quality(int quality);
    string get_device_path();
    // CameraType get_camera_type();
    void set_pipeline_element(GstElement* _element);
    shared_ptr<Camera> get_camera();

    // Takes the pipeline created by the launch string and iterates through it to
    // find the elements for configuration
    bool get_element_references();
    bool is_auto();
    void set_auto(bool mode);
};

#endif