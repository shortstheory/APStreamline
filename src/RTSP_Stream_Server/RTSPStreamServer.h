#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H

#include <map>
#include <iterator>
#include <gst/rtsp-server/rtsp-server.h>

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
    vector<string> device_list;
    map<string, RTSPAdaptiveStreaming*> adaptive_streams_map;
    map<string, v4l2_info> device_properties_map;

    void get_v4l2_devices();
    void get_v4l2_devices_info();
    void setup_streams();
    // Test the capabilities of a connected camera to see which mode it should be used in
    bool check_h264_ioctls(int fd);

public:
    RTSPStreamServer(string _ip_addr, string _port);
    ~RTSPStreamServer();
    GstRTSPServer* get_server();

    void set_service_id(guint id);
    string get_ip_address();
    string get_port();
    map<string, v4l2_info> get_device_map();
    map<string, RTSPAdaptiveStreaming*> get_stream_map();
};

#endif
