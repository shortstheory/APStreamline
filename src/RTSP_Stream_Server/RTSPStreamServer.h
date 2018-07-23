#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H

#include <map>
#include <iterator>
#include <gst/rtsp-server/rtsp-server.h>

#include "RTSPAdaptiveStreaming.h"

// Singleton class, we will never need more than one in a context
using namespace std;

class RTSPStreamServer
{
private:
    static string v4l2_device_path;
    static string v4l2_device_prefix;
    static string mount_point_prefix;
    static bool initialised;
    static RTSPStreamServer* instance;

    string ip_addr;
    string port;

    GstRTSPServer* server;

    vector<string> device_list;
    map<string, RTSPAdaptiveStreaming*> adaptive_streams_map;
    map<string, v4l2_info> device_properties_map;

    void get_v4l2_devices();
    void get_v4l2_devices_info();
    void setup_streams();
    void remove_mount_point(string mount_point);
    bool check_h264_ioctls(int fd);

    RTSPStreamServer(string _ip_addr, string _port);
    ~RTSPStreamServer();
public:
    static RTSPStreamServer* get_instance(string _ip_addr, string _port);
    GstRTSPServer* get_server();
    void remove_stream(string device);

    string get_ip_address();
    string get_port();
    map<string, v4l2_info> get_device_map();
    map<string, RTSPAdaptiveStreaming*> get_stream_map();
};

#endif
