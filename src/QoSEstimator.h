#ifndef QOS_ESTIMATOR_H
#define QOS_ESTIMATOR_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>

class QoSEstimator {
    guint32 estimated_bitrate;
public:
    QoSEstimator();
    ~QoSEstimator();
    void handle_rtcp_packet(GstRTCPPacket* packet);
    void process_rr_packet(GstRTCPPacket* packet);
    void process_sr_packet(GstRTCPPacket* packet);
};

#endif