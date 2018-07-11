#ifndef UDP_ADAPTIVE_STREAMING_H
#define UDP_ADAPTIVE_STREAMING_H

#include "GenericAdaptiveStreaming.h"

using namespace std;

class UDPAdaptiveStreaming : public GenericAdaptiveStreaming {
private:
    const gint video_sink_port;
    const gint rtcp_port;
    const string receiver_ip_addr;

    GstElement* rtpbin;
    GstElement* rr_rtcp_identity;
    GstElement* sr_rtcp_identity;
    GstElement* video_udp_sink;
    GstElement* rtcp_udp_sink;
    GstElement* rtcp_udp_src;
    string rtcp_caps_string;

    bool init_elements();
    void init_element_properties();
    bool init_rtp_elements();
    bool link_all_elements();

    void init_rtp_element_properties();
    void pipeline_add_rtp_elements();
    void rtcp_callback(GstElement* src, GstBuffer *buf);
    GstPadProbeReturn payloader_callback(GstPad* pad, GstPadProbeInfo* info);

    static void static_callback(GstElement* src, GstBuffer* buf, gpointer data);
    static GstPadProbeReturn static_payloader_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data);

public:
    UDPAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM,
                         string _ip_addr = "127.0.0.1", gint _video_port = 5000,
                         gint _rtcp_port = 5001, int quality = AUTO_PRESET);
    ~UDPAdaptiveStreaming() override;
};

#endif