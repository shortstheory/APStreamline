#include "QoSEstimator.h" 

QoSEstimator::QoSEstimator(){}

QoSEstimator::~QoSEstimator(){}

void QoSEstimator::handle_rtcp_packet(GstRTCPPacket* packet)
{
    GstRTCPType type;
    type = gst_rtcp_packet_get_type(packet);
    g_warning("pkt type %d", type);
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
    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;
    guint count, i;

    count = gst_rtcp_packet_get_rb_count(packet);
    g_debug("    count         %d", count);
    for (i=0; i<count; i++) {
        guint32 exthighestseq, jitter, lsr, dlsr;
        guint8 fractionlost;
        gint32 packetslost;
        gst_rtcp_packet_get_rb(packet, i, &ssrc, &fractionlost,
                &packetslost, &exthighestseq, &jitter, &lsr, &dlsr);
        // g_warning("    block         %llu", i);
        // g_warning("    ssrc          %llu", ssrc);
        // g_warning("    highest   seq %llu", exthighestseq);
        // g_warning("    jitter        %llu", jitter);
        // g_warning("    fraction lost %llu", fractionlost);
        // g_warning("    packet   lost %llu", packetslost);
        // g_warning("lsr %llu", lsr>>16);
    }
}

void QoSEstimator::process_sr_packet(GstRTCPPacket* packet)
{
    guint32 ssrc, rtptime, packet_count, octet_count;
    guint64 ntptime;
    gst_rtcp_packet_sr_get_sender_info(packet, &ssrc, &ntptime, &rtptime, &packet_count, &octet_count);
    // g_warning("Sender report %lu", get_compressed_ntp_time(get_ntp_time()));
    g_warning("Sender report %lu", get_ntp_time());
    // ntptime = ntptime >> 32;
    // ntptime = (ntptime & 0x0000FFFF);
    g_warning("ssrc %llu, ntptime %llu, rtptime %llu, packetcount %llu", ssrc, ntptime, rtptime, packet_count);
}

guint64 QoSEstimator::get_ntp_time()
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