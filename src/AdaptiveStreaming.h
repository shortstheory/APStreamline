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
    
    int h264_bitrate;

    GstElement* pipeline;
    GstElement* v4l2_src;
    GstElement* h264_encoder;
    GstElement* h264_parser;
    GstElement* rtph264_payloader;
    GstElement* rtpbin;
    GstElement* rr_rtcp_identity;
    GstElement* sr_rtcp_identity;
    GstElement* video_udp_sink;
    GstElement* rtcp_udp_sink;
    GstElement* rtcp_udp_src;

    GstCaps* video_caps;
    GstCaps* rtcp_caps;

    string video_caps_string;
    string rtcp_caps_string;
    static const string receiver_ip_addr;
public:
    AdaptiveStreaming();
    ~AdaptiveStreaming();
    bool init_elements();
    bool init_caps(int width, int height, int framerate);
    void init_element_properties();
    void pipeline_add_elements();
    bool link_all_elements();
};

#endif