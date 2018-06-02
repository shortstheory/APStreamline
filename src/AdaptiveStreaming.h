#ifndef ADAPTIVE_STREAMING_H
#define ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>
#include "QoSEstimator.h"

class AdaptiveStreaming {
private:
    GstElement* pipeline;
    GstElement* v4l2_src;
    GstElement* video_udp_sink;
    GstElement* h264_encoder;
    GstElement* rtph264_payloader;
    GstElement* rtpbin;
    GstElement* rr_rtcp_identity;
    GstElement* sr_rtcp_identity;
public:
    AdaptiveStreaming();
    ~AdaptiveStreaming();
};

#endif