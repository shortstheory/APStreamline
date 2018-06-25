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
    GstElement* rtp_identity;
    GstElement* rr_rtcp_identity;
    GstElement* sr_rtcp_identity;
    GstElement* video_udp_sink;
    GstElement* rtcp_udp_sink;
    GstElement* rtcp_udp_src;
    string rtcp_caps_string;

    bool init_rtp_elements();
    bool link_all_elements() override;

    void init_rtp_element_properties();
    void pipeline_add_rtp_elements();
    void rtcp_callback(GstElement* src, GstBuffer *buf);
    void rtp_callback(GstElement* src, GstBuffer* buf);

    static void static_callback(GstElement* src, GstBuffer* buf, gpointer data);
    static void static_rtp_callback(GstElement* src, GstBuffer* buf, gpointer data);
    static GstPadProbeReturn static_rtph_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data)
    {
        UDPAdaptiveStreaming* ptr = (UDPAdaptiveStreaming*)data;
        guint32 buffer_size;
        GstBuffer* buf = GST_PAD_PROBE_INFO_BUFFER(info);
        if (buf != nullptr) {
            buffer_size = gst_buffer_get_size(buf);
            // g_warning("RTPHBUFSIZE %d", buffer_size);
            guint64 bytes_sent;
            g_object_get(ptr->video_udp_sink, "bytes-served", &bytes_sent, NULL);
            ptr->qos_estimator.estimate_bandwidth(bytes_sent, buffer_size);
            // qos_estimator.estimate_rtp_pkt_size(buffer_size);
            // qos_estimator.estimate_encoding_rate(buffer_size);
        }
        return GST_PAD_PROBE_OK;
    }
    // static void 

public:
    UDPAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM,
                        string _ip_addr = "127.0.0.1", gint _video_port = 5000,
                        gint _rtcp_port = 5001);
    ~UDPAdaptiveStreaming() override;
};

#endif