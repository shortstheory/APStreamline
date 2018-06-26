#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H

#include <map>
#include <iterator>
#include <gst/rtsp-server/rtsp-server.h>

#include "RTSPAdaptiveStreaming.h"

// Singleton class, we will never need more than one in a context
using namespace std;

class RTSPStreamServer {
private:
    struct v4l2_info {
        string camera_name;
        GenericAdaptiveStreaming::CameraType camera_type;
    };

    static string v4l2_device_path;
    static string v4l2_device_prefix;
    GstRTSPServer* server;

    vector<string> device_list;
    map<string, v4l2_info> device_properties_map;

    void get_v4l2_devices();
    void get_v4l2_devices_info();

    static bool initialised;
    static RTSPStreamServer* instance;
    RTSPStreamServer(string ip_addr = "172.17.0.2", string port = "8554");
    ~RTSPStreamServer();
public:
    static RTSPStreamServer* get_instance();
};

#endif
