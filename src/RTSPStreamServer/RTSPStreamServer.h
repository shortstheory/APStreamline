#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H

#include <map>
#include <iterator>
#include <gst/rtsp-server/rtsp-server.h>
#include <memory>
#include "RTSPAdaptiveStreaming.h"

using namespace std;

// Manages the RTSP streams of a number of connected cameras
class RTSPStreamServer
{
private:
    string ip_addr;
    string port;
    guint service_id;

    GstRTSPServer* server;

    // Manage the list of streams by using maps keyed by the device name
    map<string, shared_ptr<RTSPAdaptiveStreaming>> adaptive_streams_map;

    vector<string> get_v4l2_devices_paths();
    void setup_streams();
    // Test the capabilities of a connected camera to see which mode it should be used in
    bool check_h264_ioctls(int fd);

public:
    RTSPStreamServer(string _ip_addr, string _port);
    ~RTSPStreamServer();
    GstRTSPServer* get_server();
    pair<CameraType, string> get_camera_type(const string &device);
    void set_service_id(guint id);
    string get_ip_address();
    string get_port();
    map<string, shared_ptr<RTSPAdaptiveStreaming>> get_stream_map();
};

#endif
