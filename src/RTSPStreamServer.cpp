#include "RTSPStreamServer.h"
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool RTSPStreamServer::initialised = false;
RTSPStreamServer* RTSPStreamServer::instance = nullptr;

string RTSPStreamServer::v4l2_device_path = "/dev/";
string RTSPStreamServer::v4l2_device_prefix = "video";
string RTSPStreamServer::mount_point_prefix = "/cam";

RTSPStreamServer::RTSPStreamServer(string _ip_addr, string _port) : ip_addr(_ip_addr), port(_port)
{

    server = gst_rtsp_server_new();
    gst_rtsp_server_set_address(server, ip_addr.c_str());
    gst_rtsp_server_set_service(server, port.c_str());

    get_v4l2_devices();
    get_v4l2_devices_info();
    // rtsp_adaptive_streaming = new RTSPAdaptiveStreaming("/dev/video0", GenericAdaptiveStreaming::CameraType::RAW_CAM,
    //         "/webcam", server);
    setup_streams();
}

// convert strings to char arrays here, this is sub-optimal
void RTSPStreamServer::get_v4l2_devices()
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir(v4l2_device_path.c_str());
    if (dp == NULL) {
        fprintf(stderr, "Could not open directory %d", errno);
    }
    while ((ep = readdir(dp))) {
        // fprintf(stderr, "%s\n", ep->d_name);
        string s = ep->d_name;
        if (s.find(v4l2_device_prefix) != std::string::npos) {
            s = v4l2_device_path + s;
            fprintf(stderr, "Found V4L2 camera device %s\n", s.c_str());
            // add device path to list
            device_list.push_back(s);
        }
    }
    closedir(dp);
}

void RTSPStreamServer::get_v4l2_devices_info()
{
    for (string dev : device_list) {
        fprintf(stderr, "%s\n", dev.c_str());
        int fd = open(dev.c_str(), O_RDONLY);
        v4l2_info info;
        if (fd != -1) {
            v4l2_capability caps;
            ioctl(fd, VIDIOC_QUERYCAP, &caps);
            info.camera_name = string(caps.card, caps.card + sizeof caps.card / sizeof caps.card[0]);
            info.camera_type = GenericAdaptiveStreaming::CameraType::RAW_CAM;
            device_properties_map.insert(pair<string, v4l2_info>(dev, info));
            fprintf(stderr, "name - %s driver - %s\n", caps.card, caps.driver);
            close(fd);
        }
    }
}

void RTSPStreamServer::setup_streams()
{
    int i;
    i = 0;
    for (auto it = device_properties_map.begin(); it != device_properties_map.end(); it++, i++) {
        adaptive_streams.push_back(new RTSPAdaptiveStreaming(it->first, it->second.camera_type,
                                    mount_point_prefix+to_string(i), server));
    }
}

GstRTSPServer* RTSPStreamServer::get_server()
{
    return server;
}

RTSPStreamServer* RTSPStreamServer::get_instance()
{
    if (!initialised) {
        instance = new RTSPStreamServer();
        initialised = true;
    }
    return instance;
}

RTSPStreamServer::~RTSPStreamServer()
{
    free(instance);
    initialised = false;
}