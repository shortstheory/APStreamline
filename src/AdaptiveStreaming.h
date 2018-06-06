#ifndef ADAPTIVE_STREAMING_H
#define ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>
#include "QoSEstimator.h"

using namespace std;

class AdaptiveStreaming {
private:
    static const int video_sink_port = 5000;
    static const int rtcp_sink_port = 5001;
    static const int rtcp_src_port = 5005;
    
    guint32 h264_bitrate;

    GstElement* pipeline;
    GstElement* v4l2_src;
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

    GstCaps* video_caps;
    GstCaps* rtcp_caps;

    string video_caps_string;
    string rtcp_caps_string;
    QoSEstimator qos_estimator;

    static const string receiver_ip_addr;

    bool init_elements();
    bool init_caps(int width, int height, int framerate);
    void init_element_properties();
    void pipeline_add_elements();
    bool link_all_elements();
    void rtcp_callback(GstElement* src, GstBuffer *buf);
    void rtp_callback(GstElement* src, GstBuffer* buf);
    void adapt_stream(QoSReport report);
    static void static_callback(GstElement *src, GstBuffer *buf, gpointer data);
    static void static_rtp_callback(GstElement *src, GstBuffer *buf, gpointer data);

public:
    AdaptiveStreaming();
    ~AdaptiveStreaming();
    bool start_playing();
    GstBus* get_pipeline_bus();
};

#endif