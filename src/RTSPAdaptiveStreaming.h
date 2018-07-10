#ifndef RTSP_ADAPTIVE_STREAMING_H
#define RTSP_ADAPTIVE_STREAMING_H

#include "GenericAdaptiveStreaming.h"
#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

class RTSPAdaptiveStreaming : public GenericAdaptiveStreaming {
private:
    GstRTSPServer* rtsp_server;
    GstElement* rtpbin;
    GstElement* multi_udp_sink;
    const string uri;

    void init_media_factory();
    void add_rtpbin_probes();
    void media_prepared_callback(GstRTSPMedia* media);
    void change_quality(int quality);

    GstPadProbeReturn rtcp_callback(GstPad* pad, GstPadProbeInfo* info);
    GstPadProbeReturn payloader_callback(GstPad* pad, GstPadProbeInfo* info);

    static void static_media_constructed_callback(GstRTSPMediaFactory *media_factory, GstRTSPMedia *media,
            gpointer data);
    static void static_media_prepared_callback(GstRTSPMedia* media, gpointer user_data);
    static GstPadProbeReturn static_rtcp_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    static GstPadProbeReturn static_payloader_callback(GstPad* pad, GstPadProbeInfo* info, gpointer data);

public:
    RTSPAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM,
                          string _uri = "/test", GstRTSPServer* server = nullptr, int quality = AUTO_PRESET);
    ~RTSPAdaptiveStreaming() override;
};

#endif