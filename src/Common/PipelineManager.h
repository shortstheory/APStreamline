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
#include "../Camera/CameraType.h"
#include "QoSEstimator.h"
#include "Constants.h"

using namespace std;

// Generic implementation of an Adaptive Video Streamer. Contains all the necessary
// GstElements which need to be manually referenced by subclassing it.
// All the adapatation logic is handled here

class PipelineManager
{
private:
    shared_ptr<Camera> cam;
    bool congested;
    guint32 successive_transmissions;
    bool auto_mode;

public:
    guint32 h264_bitrate;

    GstElement* pipeline;
    GstElement* multi_udp_sink;
    GstElement* rtph264_payloader;
    GstElement* tee;

    QoSEstimator qos_estimator;

    PipelineManager(string _device = "/dev/video0", CameraType type = CameraType::MJPG_CAM);

    void adapt_stream();
    string get_device_path();
    void set_pipeline_element(GstElement* _element);
    shared_ptr<Camera> get_camera();

    // Takes the pipeline created by the launch string and iterates through it to
    // find the elements for configuration
    bool get_element_references();
    bool is_auto();
    void set_auto(bool mode);
};

#endif