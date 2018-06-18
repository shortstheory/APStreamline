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

class AdaptiveStreaming {
private:
    enum ResolutionPresets {LOW, MED, HIGH} current_res;

    const gint video_sink_port;
    const gint rtcp_port;

    const int bitrate_inc = 250;
    const int bitrate_dec = 1000;

    static const int max_bitrate = 8000;
    static const int min_bitrate = 50;

    const string device;
    const string receiver_ip_addr;

    guint32 h264_bitrate;

    gint v4l2_cam_fd;

    GstElement* pipeline;
    GstElement* v4l2_src;
    GstElement* src_capsfilter;
    GstElement* videoconvert;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;
    GstElement* rtpbin;
    GstElement* rtp_identity;
    GstElement* rr_rtcp_identity;
    GstElement* sr_rtcp_identity;
    GstElement* video_udp_sink;
    GstElement* rtcp_udp_sink;
    GstElement* rtcp_udp_src;

    string video_caps_string;
    string rtcp_caps_string;
    QoSEstimator qos_estimator;

    // better off as a char array, change it later
    string video_presets[3];
    guint32 bitrate_presets[3];

    bool init_elements();
    void init_element_properties();
    void pipeline_add_elements();
    bool link_all_elements();
    void rtcp_callback(GstElement* src, GstBuffer *buf);
    void rtp_callback(GstElement* src, GstBuffer* buf);
    void adapt_stream();
    void set_encoding_bitrate(guint32 bitrate);
    void set_resolution(ResolutionPresets setting);
    void increase_resolution();
    void decrease_resolution();

    void improve_quality();
    void degrade_quality();

    static void static_callback(GstElement *src, GstBuffer *buf, gpointer data);
    static void static_rtp_callback(GstElement *src, GstBuffer *buf, gpointer data);

public:
    enum CameraType {RAW_CAM, H264_CAM};
    const CameraType camera_type;

    AdaptiveStreaming(string _device = "/dev/video0", string _ip_addr = "127.0.0.1",
                      CameraType type = CameraType::RAW_CAM, gint _video_port = 5000,
                      gint _rtcp_port = 5001);

    ~AdaptiveStreaming();
    bool play_pipeline();
    bool pause_pipeline();
    bool change_source(string _device);
    GstBus* get_pipeline_bus();
};

#endif