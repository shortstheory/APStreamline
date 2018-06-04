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
            result.fraction = full_ntp_timestamp & 0x00000000FFFFFFFF;
            result.second = (full_ntp_timestamp & 0xFFFFFFFF00000000) >> 32;
            return result;
        }

        static ntp_time_t get_struct_from_compressed_timestamp(guint32 compressed_ntp_timestamp)
        {
            ntp_time_t result;
            result.second = (compressed_ntp_timestamp & 0xFFFF0000) >> 16;
            result.fraction = compressed_ntp_timestamp & 0x0000FFFF;
            return result;
        }

        gfloat calculate_difference(guint32 compressed_ntp_timestamp)
        {
            guint32 compressed_second;
            gint32 compressed_fraction;

            // last 16 bits of second field, first 16 bits of fraction field
            compressed_second = (second & 0x0000FFFF);
            compressed_fraction = (fraction & 0xFFFF0000) >> 16;

            guint32 ts_compressed_second;
            gint32 ts_compressed_fraction;

            //upper 16 bits is seconds, lower 16 bits is fraction
            ts_compressed_second = (compressed_ntp_timestamp & 0xFFFF0000) >> 16;
            ts_compressed_fraction = (compressed_ntp_timestamp & 0x0000FFFF);

            gfloat time_delta;

            time_delta = (compressed_second - ts_compressed_second) + (float)(compressed_fraction - ts_compressed_fraction) * 1 / 65536.0; 
            return time_delta;
        }

        static gfloat calculate_compresssed_timestamp_diff(guint32 ts_1, guint ts_2)
        {
            ntp_time_t time1;
            ntp_time_t time2;
            time1 = ntp_time_t::get_struct_from_compressed_timestamp(ts_1);
            time2 = ntp_time_t::get_struct_from_compressed_timestamp(ts_2);;
            gfloat time_delta;

            time_delta = (time2.second - time1.second) + (float)(time2.fraction - time1.fraction) * 1 / 65536.0;
            return time_delta;
        }
    };
    static const guint64 ntp_offset = 2208988800;

    guint32 estimated_bitrate;
    guint32 prev_rr_time;
    gfloat smooth_rtt;

    guint64 get_current_ntp_time();
    guint32 get_compressed_ntp_time(guint64 full_ntp_timestamp);
    static void exp_smooth_val(gfloat curr_val, gfloat &smooth_val, gfloat alpha);
public:
    QoSEstimator();
    ~QoSEstimator();
    void handle_rtcp_packet(GstRTCPPacket* packet);
    void process_rr_packet(GstRTCPPacket* packet);
    void process_sr_packet(GstRTCPPacket* packet);
};

#endif