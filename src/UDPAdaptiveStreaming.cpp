#include "UDPAdaptiveStreaming.h"

UDPAdaptiveStreaming::UDPAdaptiveStreaming(string _device, string _ip_addr, CameraType type,
                                            gint _video_port, gint _rtcp_port) : 
                                            GenericAdaptiveStreaming(device, type), 
                                            receiver_ip_addr(_ip_addr), video_sink_port(_video_port), 
                                            rtcp_port(_rtcp_port)
{

}

bool UDPAdaptiveStreaming::init_rtp_elements()
{
    rtpbin = gst_element_factory_make("rtpbin", NULL);
    rtp_identity = gst_element_factory_make("identity", NULL);
    rr_rtcp_identity = gst_element_factory_make("identity", NULL);
    sr_rtcp_identity = gst_element_factory_make("identity", NULL);
    rtcp_udp_src = gst_element_factory_make("udpsrc", NULL);
    video_udp_sink = gst_element_factory_make("udpsink", NULL);
    rtcp_udp_sink = gst_element_factory_make("udpsink", NULL);

    if (!rtpbin && !rr_rtcp_identity && !sr_rtcp_identity && !rtcp_udp_src && !video_udp_sink && 
        !rtcp_udp_sink && !rtp_identity) {
        return false;
    }
    return true;
}
