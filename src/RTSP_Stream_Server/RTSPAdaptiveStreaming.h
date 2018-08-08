#ifndef RTSP_ADAPTIVE_STREAMING_H
#define RTSP_ADAPTIVE_STREAMING_H

#include <gst/gst.h>
#include <gst/rtp/gstrtcpbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>

#include "../Common/Common.h"

using namespace std;

// Subclass of GenericAdaptiveStreaming, handling the streaming over RTSP

class RTSPAdaptiveStreaming : public GenericAdaptiveStreaming
{
private:
    const string uri;
    GstRTSPServer* rtsp_server;
    GstRTSPMediaFactory* media_factory;

    bool media_prepared;

    GstElement* rtpbin;

    void init_media_factory();
    void add_rtpbin_probes();

    // Several GStreamer functions operate using callbacks, however we need to first
    // catch them as static methods while passing the 'this' pointer as an argument
    // to call the object's method!
    void media_prepared_callback(GstRTSPMedia* media);
    void media_unprepared_callback(GstRTSPMedia* media);
    void deep_callback(GstBin* bin,
                       GstBin* sub_bin,
                       GstElement* element);
    GstPadProbeReturn rtcp_callback(GstPad* pad, GstPadProbeInfo* info);
    GstPadProbeReturn probe_block_callback(GstPad* pad, GstPadProbeInfo* info);
    GstPadProbeReturn payloader_callback(GstPad* pad, GstPadProbeInfo* info);

    static void static_media_constructed_callback(GstRTSPMediaFactory *media_factory,
            GstRTSPMedia *media,
            gpointer data);
    static void static_media_prepared_callback(GstRTSPMedia* media, gpointer data);
    static void static_media_unprepared_callback(GstRTSPMedia* media, gpointer data);
    static void static_deep_callback(GstBin* bin,
                                     GstBin* sub_bin,
                                     GstElement* element,
                                     gpointer data);
    static GstPadProbeReturn static_probe_block_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);
    static GstPadProbeReturn static_rtcp_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);
    static GstPadProbeReturn static_payloader_callback(GstPad* pad,
            GstPadProbeInfo* info,
            gpointer data);

    // Takes the pipeline created by the launch string and iterates through it to
    // find the elements for configuration
    bool get_element_references();
    void record_stream(bool _record_stream);

public:
    RTSPAdaptiveStreaming(string _device = "/dev/video0",
                          CameraType type = CameraType::RAW_CAM,
                          string _uri = "/test", GstRTSPServer* server = nullptr,
                          int quality = AUTO_PRESET);
    ~RTSPAdaptiveStreaming() override;
    void set_device_properties(int quality, bool record_stream);
    bool get_media_prepared();
};

#endif