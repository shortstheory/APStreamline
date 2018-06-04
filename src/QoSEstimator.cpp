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

}

void QoSEstimator::process_sr_packet(GstRTCPPacket* packet)
{

}
