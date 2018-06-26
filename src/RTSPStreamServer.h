#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H
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

    vector<string> v4l2_device_list;

    void get_v4l2_devices();
    void get_v4l2_devices_info();

    static bool initialised;
    static RTSPStreamServer* instance;
    RTSPStreamServer();
    ~RTSPStreamServer();
public:
    static RTSPStreamServer* get_instance();
};

#endif
