#include "QoSEstimator.h"

QoSEstimator::QoSEstimator() : smooth_rtt(0), prev_rr_time(0), prev_pkt_count(0),
    prev_buffer_occ(0), rtp_size(0), bytes_transferred(0),
    smooth_enc_bitrate(0), last_bytes_sent(0), rtph_bytes_interval(0)
{
    gettimeofday(&prev_tv, NULL);
    gettimeofday(&prev_bw_tv, NULL);
}

QoSEstimator::~QoSEstimator() {}

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
    guint64 curr_time_ms = 0;

    guint32 exthighestseq;
    guint32 jitter;
    guint32 lsr;
    guint32 dlsr;
    guint32 ssrc;
    gint32 packetslost;

    guint8 fractionlost;
    gfloat curr_buffer_occ = 0;

    gst_rtcp_packet_get_rb(packet, 0, &ssrc, &fractionlost,
                           &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);
    timeval tv;
    gettimeofday(&tv, NULL);
    // rtt calc
    update_rtt(lsr, dlsr);
    prev_rr_time = curr_time_ms;
    qos_report = QoSReport(fractionlost, estimated_bitrate, encoding_bitrate, smooth_rtt, curr_buffer_occ);
}

QoSReport QoSEstimator::get_qos_report()
{
    return qos_report;
}

void QoSEstimator::process_sr_packet(GstRTCPPacket* packet)
{
    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;
    gst_rtcp_packet_sr_get_sender_info(packet, &ssrc, &ntptime, &rtptime, &packet_count, &octet_count);
}

void QoSEstimator::exp_smooth_val(const gfloat &curr_val, gfloat &smooth_val, gfloat alpha)
{
    smooth_val = curr_val * alpha + (1 - alpha) * smooth_val;
}

void QoSEstimator::calculate_bitrates(const guint64 &bytes_sent, const guint32 &buffer_size)
{
    guint64 last_count = ntp_time_t::unix_time_to_ms(prev_bw_tv);
    timeval tv;
    gettimeofday(&tv, NULL);
    guint64 curr_count = ntp_time_t::unix_time_to_ms(tv);
    guint64 bytes_interval;
    if (curr_count - last_count > 1000) {
        bytes_interval = bytes_sent - last_bytes_sent;
        estimated_bitrate = bytes_interval * 8.0 / (float)(curr_count - last_count);
        encoding_bitrate = rtph_bytes_interval * 8.0 / (float)(curr_count - last_count);
        prev_bw_tv = tv;
        last_bytes_sent = bytes_sent;
        rtph_bytes_interval = 0;
    } else {
        rtph_bytes_interval += buffer_size;
    }
}

gfloat QoSEstimator::update_rtt(const guint32 &lsr, const guint32 &dlsr)
{
    gfloat curr_rtt;
    timeval tv;
    gettimeofday(&tv, NULL);
    ntp_time_t curr_time = ntp_time_t::convert_from_unix_time(tv);
    gfloat timediff = ntp_time_t::calculate_difference(curr_time, lsr);
    curr_rtt = timediff - dlsr * 1/65535.0;
    if (curr_rtt < 1) {
        exp_smooth_val(curr_rtt, smooth_rtt, 0.80);
    }
    return curr_rtt;
}