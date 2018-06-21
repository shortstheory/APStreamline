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
    GstRTSPMediaFactory* media_factory;
    GstRTSPServer* rtsp_server;

    const string uri;

    bool link_all_elements() override;
    void init_media_factory();

    static GstElement* create_custom_pipeline(GstRTSPMediaFactory * factory, const GstRTSPUrl  *url);

public:
    RTSPAdaptiveStreaming(string _device = "/dev/video0", CameraType type = CameraType::RAW_CAM,
                        string _uri = "test", GstRTSPServer* server = nullptr);
    ~RTSPAdaptiveStreaming() override;
};

#endif