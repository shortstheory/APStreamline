#ifndef RTSP_ADAPTIVE_STREAMING_H
#define RTSP_ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <unistd.h>
#include <stdlib.h>
#include <condition_variable>
#include <mutex>

#include "GenericAdaptiveStreaming.h"

using namespace std;

class RTSPAdaptiveStreaming : public GenericAdaptiveStreaming
{
private:
    mutex mtx;
    condition_variable cv;
    GstRTSPServer* rtsp_server;
    GstElement* rtpbin;
    const string uri;
    bool media_prepared;

    void init_media_factory();
    void add_rtpbin_probes();
    void media_prepared_callback(GstRTSPMedia* media);

    GstPadProbeReturn rtcp_callback(GstPad* pad, GstPadProbeInfo* info);
    GstPadProbeReturn payloader_callback(GstPad* pad, GstPadProbeInfo* info);
    GstPadProbeReturn probe_block_callback(GstPad* pad, GstPadProbeInfo* info);

    static void static_media_constructed_callback(GstRTSPMediaFactory *media_factory,
            GstRTSPMedia *media,
            gpointer data);
    static void static_media_prepared_callback(GstRTSPMedia* media, gpointer user_data);
    static GstPadProbeReturn static_probe_block_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);
    static GstPadProbeReturn static_rtcp_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);
    static GstPadProbeReturn static_payloader_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);
    void record_stream(bool _record_stream);

public:
    FileRecorder file_recorder;
    RTSPAdaptiveStreaming(string _device = "/dev/video0",
                          CameraType type = CameraType::RAW_CAM,
                          string _uri = "/test", GstRTSPServer* server = nullptr,
                          int quality = AUTO_PRESET);
    ~RTSPAdaptiveStreaming() override;
    void set_device_properties(int quality, bool record_stream);
    bool get_media_prepared();
};

#endif