#include "RTSPStreamServer.h"
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <dirent.h>
#include <string>
#include <string.h>

bool RTSPStreamServer::initialised = true;
RTSPStreamServer* RTSPStreamServer::instance = nullptr;

string RTSPStreamServer::v4l2_device_path = "/dev/";
string RTSPStreamServer::v4l2_device_prefix = "video";

RTSPStreamServer::RTSPStreamServer()
{

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
        if (strncmp(v4l2_device_prefix.c_str(), ep->d_name, sizeof(v4l2_device_prefix.c_str()) - 1) == 0) {
            fprintf(stderr, "Found V4L2 camera device %s", ep->d_name);
            // add device path to list
            v4l2_device_list.push_back(v4l2_device_prefix + ep->d_name);
        }
    }
    closedir(dp);
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