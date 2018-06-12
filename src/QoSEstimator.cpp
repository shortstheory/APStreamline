#include "QoSEstimator.h" 

QoSEstimator::QoSEstimator() : smooth_rtt(0), prev_rr_time(0), prev_pkt_count(0),
                                prev_buffer_occ(0), rtp_size(0), bytes_transferred(0),
                                smooth_enc_bitrate(0)
{
    gettimeofday(&prev_tv, NULL);
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
    guint64 rr_time_delta_ms;
    guint64 curr_time_ms;
    gfloat bandwidth;

    guint32 exthighestseq, jitter, lsr, dlsr;
    guint32 packet_interval;
    guint32 ssrc;
    gint32 packetslost;

    guint8 fractionlost;
    gfloat curr_buffer_occ;
    gfloat curr_rtt;

    gst_rtcp_packet_get_rb(packet, 0, &ssrc, &fractionlost,
            &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);
    // rtt calc
    timeval tv;
    gettimeofday(&tv, NULL);
    ntp_time_t curr_time = ntp_time_t::convert_from_unix_time(tv);
    gfloat timediff = curr_time.calculate_difference(lsr);
    curr_rtt = timediff - dlsr*1/65535.0;
    if (curr_rtt < 1) {
        exp_smooth_val(curr_rtt, smooth_rtt, 0.80);
    }
    // b/w estd
    curr_time_ms = (tv.tv_sec * (uint64_t)1000) + (tv.tv_usec / 1000);
    packet_interval = exthighestseq - prev_pkt_count;
    rr_time_delta_ms = curr_time_ms - prev_rr_time;
    bandwidth = (packet_interval * rtp_size) * 8.0 / (float)rr_time_delta_ms;
    exp_smooth_val(bandwidth, estimated_bitrate, 0.75);

    curr_buffer_occ = prev_buffer_occ + (smooth_enc_bitrate - estimated_bitrate) * curr_rtt;

    prev_pkt_count = exthighestseq;
    prev_rr_time = curr_time_ms;

    qos_report = QoSReport(fractionlost, estimated_bitrate, smooth_enc_bitrate, smooth_rtt, curr_buffer_occ);

    g_warning("bw %f occ %f loss %d encode-Rate %f", bandwidth, curr_buffer_occ, fractionlost, smooth_enc_bitrate);

    // g_warning("rtt %f rtpsize %f encode-Rate %f", smooth_rtt, rtp_size, smooth_enc_bitrate);
        // g_warning("    block         %llu", i);
        // g_warning("    ssrc          %llu", ssrc);
        // g_warning("    highest   seq %llu", exthighestseq);
        // g_warning("    jitter        %llu", jitter);
        // g_warning("    fraction lost %llu", fractionlost);
        // g_warning("    packet   lost %llu", packetslost);
        // g_warning("lsr %llu", lsr>>16);
}

QoSReport QoSEstimator::get_qos_report()
{
    return qos_report;
}

// not really used anywhere yet, but we'll keep it around till it becomes useful
void QoSEstimator::process_sr_packet(GstRTCPPacket* packet)
{
    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;
    gst_rtcp_packet_sr_get_sender_info(packet, &ssrc, &ntptime, &rtptime, &packet_count, &octet_count);
    // g_warning("t1 s%lu f%lu t2 s%lu f%lu", t1.second, t1.fraction, t2.second, t2.fraction);
    // g_warning("Sender report %lu", get_compressed_ntp_time(get_ntp_time()));
    // g_warning("Sender report %lu", get_ntp_time());
    // ntptime = ntptime >> 32;
    // ntptime = (ntptime & 0x0000FFFF);

    // g_warning("ssrc %llu, ntptime %llu, rtptime %llu, packetcount %llu", ssrc, ntptime, rtptime, packet_count);
}

void QoSEstimator::exp_smooth_val(const gfloat &curr_val, gfloat &smooth_val, gfloat alpha)
{
    smooth_val = curr_val * alpha + (1 - alpha) * smooth_val;
}

void QoSEstimator::estimate_encoding_rate(const guint32 &pkt_size)
{
    // g_warning("BUFSIZE %lu", pkt_size);
    guint64 last_count = ntp_time_t::unix_time_to_ms(prev_tv);
    timeval tv;
    gettimeofday(&tv, NULL);
    guint64 curr_count = ntp_time_t::unix_time_to_ms(tv);
    if (curr_count - last_count > 1000) {
        encoding_bitrate = (bytes_transferred) * 8.0 / (float)(curr_count - last_count);
        // g_warning("b %f", encoding_bitrate);
        prev_tv = tv;
        bytes_transferred = 0;
        exp_smooth_val(encoding_bitrate, smooth_enc_bitrate, 0.75);
    } else {
        bytes_transferred += pkt_size;
        // g_warning("b %llu", bytes_transferred);
    }
}

void QoSEstimator::estimate_rtp_pkt_size(const guint32 &pkt_size)
{

    exp_smooth_val(pkt_size, rtp_size, 0.25);
    // g_warning("rtpPkt: %f", rtp_size);
}