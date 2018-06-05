#include "QoSEstimator.h" 

QoSEstimator::QoSEstimator() : smooth_rtt(0), prev_rr_time(0), prev_pkt_count(0)
{
}

QoSEstimator::QoSEstimator(guint32* bitrate) : smooth_rtt(0), prev_rr_time(0), 
                                                prev_pkt_count(0), h264_bitrate(bitrate)
{
}

QoSEstimator::~QoSEstimator(){}

void QoSEstimator::handle_rtcp_packet(GstRTCPPacket* packet)
{
    GstRTCPType type;
    type = gst_rtcp_packet_get_type(packet);
    // g_warning("pkt type %d", type);
    switch (type) {
    case GST_RTCP_TYPE_RR:
        process_rr_packet(packet);
        break;
    case GST_RTCP_TYPE_SR:
        process_sr_packet(packet);
        break;
    default:
        break;
    }
}

void QoSEstimator::process_rr_packet(GstRTCPPacket* packet)
{
    guint32 packet_interval;
    guint64 rr_time_delta_ms;
    guint64 curr_time_ms;
    gfloat bandwidth;

    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;

    guint32 exthighestseq, jitter, lsr, dlsr;
    guint8 fractionlost;
    gint32 packetslost;
    gst_rtcp_packet_get_rb(packet, 0, &ssrc, &fractionlost,
            &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);
    // rtt calc
    timeval tv;
    gettimeofday(&tv, NULL);
    ntp_time_t curr_time = ntp_time_t::convert_from_unix_time(tv);
    gfloat timediff = curr_time.calculate_difference(lsr);
    exp_smooth_val(timediff - dlsr*1/65535.0, smooth_rtt, 0.75);

    // b/w estd
    curr_time_ms = (tv.tv_sec * (uint64_t)1000) + (tv.tv_usec / 1000);
    packet_interval = exthighestseq - prev_pkt_count;
    rr_time_delta_ms = curr_time_ms - prev_rr_time;
    bandwidth = (packet_interval * rtp_size) * 8.0 / (float)rr_time_delta_ms;
    exp_smooth_val(bandwidth, estimated_bitrate, 0.75);

    prev_pkt_count = exthighestseq;
    prev_rr_time = curr_time_ms;

    g_warning("bw %f %llu %llu %llu", estimated_bitrate, curr_time_ms, prev_rr_time, rr_time_delta_ms);

    g_warning("rtt %f ", smooth_rtt);
        // g_warning("    block         %llu", i);
        // g_warning("    ssrc          %llu", ssrc);
        // g_warning("    highest   seq %llu", exthighestseq);
        // g_warning("    jitter        %llu", jitter);
        // g_warning("    fraction lost %llu", fractionlost);
        // g_warning("    packet   lost %llu", packetslost);
        // g_warning("lsr %llu", lsr>>16);
}

void QoSEstimator::process_sr_packet(GstRTCPPacket* packet)
{
    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;
    gst_rtcp_packet_sr_get_sender_info(packet, &ssrc, &ntptime, &rtptime, &packet_count, &octet_count);
    timeval tv;
    gettimeofday(&tv, NULL);
    ntp_time_t t1 = ntp_time_t::convert_from_unix_time(tv);
    ntp_time_t t2 = ntp_time_t::get_struct_from_timestamp(ntptime);
    // g_warning("t1 s%lu f%lu t2 s%lu f%lu", t1.second, t1.fraction, t2.second, t2.fraction);
    // g_warning("Sender report %lu", get_compressed_ntp_time(get_ntp_time()));
    // g_warning("Sender report %lu", get_ntp_time());
    // ntptime = ntptime >> 32;
    // ntptime = (ntptime & 0x0000FFFF);

    // g_warning("ssrc %llu, ntptime %llu, rtptime %llu, packetcount %llu", ssrc, ntptime, rtptime, packet_count);
}

guint64 QoSEstimator::get_current_ntp_time()
{
    return time(NULL) + ntp_offset;
}

guint32 QoSEstimator::get_compressed_ntp_time(guint64 full_ntp_timestamp)
{
    guint32 fractional_time;
    guint32 integral_time;
    guint32 result_time;

    fractional_time = full_ntp_timestamp >> 16;
    fractional_time = fractional_time & 0x0000FFFF;

    integral_time = full_ntp_timestamp >> 32;
    integral_time = integral_time & 0x0000FFFF;

    result_time = (integral_time << 16) + (fractional_time);
    return integral_time;
}

void QoSEstimator::exp_smooth_val(gfloat curr_val, gfloat &smooth_val, gfloat alpha)
{
    smooth_val = curr_val * alpha + (1 - alpha) * smooth_val;
}