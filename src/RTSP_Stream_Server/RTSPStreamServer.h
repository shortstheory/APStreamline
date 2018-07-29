#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H

#include <map>
#include <iterator>
#include <gst/rtsp-server/rtsp-server.h>

#include "RTSPAdaptiveStreaming.h"

using namespace std;

// Manages the RTSP streams of a number of connected cameras
// Singleton class, as we will never need more than one in a context
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

    // Manage the list of streams by using maps keyed by the device name
    vector<string> device_list;
    map<string, RTSPAdaptiveStreaming*> adaptive_streams_map;
    map<string, v4l2_info> device_properties_map;

    void get_v4l2_devices();
    void get_v4l2_devices_info();
    void setup_streams();
    void remove_mount_point(string mount_point);
    // Test the capabilities of a connected camera to see which mode it should be used in
    bool check_h264_ioctls(int fd);

    RTSPStreamServer(string _ip_addr, string _port);
    ~RTSPStreamServer();
public:
    static RTSPStreamServer* get_instance(string _ip_addr, string _port);
    static RTSPStreamServer* get_instance();
    GstRTSPServer* get_server();

    void remove_stream(string device);
    string get_ip_address();
    string get_port();
    map<string, v4l2_info> get_device_map();
    map<string, RTSPAdaptiveStreaming*> get_stream_map();
};

#endif
