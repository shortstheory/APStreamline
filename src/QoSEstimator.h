#ifndef QOS_ESTIMATOR_H
#define QOS_ESTIMATOR_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <gst/rtp/gstrtcpbuffer.h>
#include <sys/time.h>

class QoSEstimator {
    struct ntp_time_t {
        guint32 second;
        guint32 fraction;

        static ntp_time_t convert_from_unix_time(timeval unix_time)
        {
            ntp_time_t result;
            result.second = unix_time.tv_sec + 0x83AA7E80;
            result.fraction = (uint32_t)((double)(unix_time.tv_usec + 1) * (double)(1LL<<32) * 1.0e-6);
            return result;
        }

        static ntp_time_t get_struct_from_timestamp(guint64 full_ntp_timestamp)
        {
            ntp_time_t result;
            result.second = (full_ntp_timestamp & 0xFFFF0000) >> 32;
            result.fraction = (full_ntp_timestamp & 0x0000FFFF) >> 32;
            return result;
        }
    };
    static const guint64 ntp_offset = 2208988800;

    guint32 estimated_bitrate;
    guint64 get_current_ntp_time();
    guint32 get_compressed_ntp_time(guint64 full_ntp_timestamp);
public:
    QoSEstimator();
    ~QoSEstimator();
    void handle_rtcp_packet(GstRTCPPacket* packet);
    void process_rr_packet(GstRTCPPacket* packet);
    void process_sr_packet(GstRTCPPacket* packet);
};

#endif