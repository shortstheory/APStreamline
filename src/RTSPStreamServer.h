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
    static string mount_point_prefix;

    string ip_addr;
    string port;

    GstRTSPServer* server;

    vector<string> device_list;
    vector<RTSPAdaptiveStreaming> adaptive_streams;

    map<string, v4l2_info> device_properties_map;

    void get_v4l2_devices();
    void get_v4l2_devices_info();
    void setup_streams();

    static bool initialised;
    static RTSPStreamServer* instance;
    RTSPStreamServer(string _ip_addr = "172.17.0.2", string _port = "8554");
    ~RTSPStreamServer();
public:
    static RTSPStreamServer* get_instance();
    GstRTSPServer* get_server();
};

#endif
