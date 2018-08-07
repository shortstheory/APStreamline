#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <dirent.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <algorithm>

#include "RTSPStreamServer.h"

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
    setup_streams();
}

void RTSPStreamServer::get_v4l2_devices()
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir(v4l2_device_path.c_str());
    if (dp == nullptr) {
        cerr << "Could not open directory " << errno << endl;
    }
    while ((ep = readdir(dp))) {
        string s = ep->d_name;
        if (s.find(v4l2_device_prefix) != std::string::npos) {
            s = v4l2_device_path + s;
            cout << "Found V4L2 camera device " << s << endl;
            // Add device path to list
            device_list.push_back(s);
        }
    }
    closedir(dp);
    // To make mount points and the device names in the same order
    reverse(device_list.begin(), device_list.end());
}

bool RTSPStreamServer::check_h264_ioctls(int fd)
{
    v4l2_queryctrl bitrate_ctrl;
    bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;

    if (ioctl(fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1) {
        return false;
    }
    return true;
}

void RTSPStreamServer::get_v4l2_devices_info()
{
    int i = 0;
    for (string dev : device_list) {
        cout << dev << endl;
        int fd = open(dev.c_str(), O_RDONLY);
        v4l2_info info;
        if (fd != -1) {
            v4l2_capability caps;
            ioctl(fd, VIDIOC_QUERYCAP, &caps);

            info.camera_name = string(caps.card, caps.card + sizeof caps.card / sizeof caps.card[0]);
            info.mount_point = mount_point_prefix + to_string(i);
            info.frame_property_bitmask = 0;

            cout << "Name - " << caps.card << " Driver - " << caps.driver << endl;

            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            v4l2_fmtdesc fmt;
            v4l2_frmsizeenum frmsize;
            v4l2_frmivalenum frmival;

            memset(&fmt, 0, sizeof(fmt));
            memset(&frmsize, 0, sizeof(frmsize));
            memset(&frmival, 0, sizeof(frmival));

            int mjpg_index = -1;
            int h264_index = -1;

            fmt.index = 0;
            fmt.type = type;

            while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
                if (!strcmp((char*)fmt.description, "Motion-JPEG")) {
                    mjpg_index = fmt.index;
                }
                if (!strcmp((char*)fmt.description, "H264") || !strcmp((char*)fmt.description, "H.264")) {
                    h264_index = fmt.index;
                }
                fmt.index++;
            }

            if (mjpg_index != -1) {
                fmt.index = mjpg_index;
            }

            if (h264_index != -1) {
                fmt.index = h264_index;
                if (check_h264_ioctls(fd)) {
                    info.camera_type = H264_CAM;
                } else {
                    info.camera_type = UVC_CAM;
                }
            } else {
                info.camera_type = RAW_CAM;
            }

            frmsize.pixel_format = fmt.pixelformat;

            for (frmsize.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0; frmsize.index++) {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                    FramePresets preset;
                    if (frmsize.discrete.width == 320 && frmsize.discrete.height == 240) {
                        preset = FRAME_320x240;
                    } else if (frmsize.discrete.width == 640 && frmsize.discrete.height == 480) {
                        preset = FRAME_640x480;
                    } else if (frmsize.discrete.width == 1280 && frmsize.discrete.height == 720) {
                        preset = FRAME_1280x720;
                    } else {
                        continue;
                    }

                    frmival.pixel_format = fmt.pixelformat;
                    frmival.width = frmsize.discrete.width;
                    frmival.height = frmsize.discrete.height;

                    for (frmival.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0; frmival.index++) {
                        int framerate  = frmival.discrete.denominator;
                        // For simplicity in maintenance, we only support 240p, 480p
                        // and 720p. If a camera supports a resolution we use a bitmask
                        // for saving its capabilities as it greatly simplifies our IPC
                        switch (preset) {
                        case FRAME_320x240:
                            if (framerate == 15) {
                                info.frame_property_bitmask |= (1 << VIDEO_320x240x15);
                            } else if (framerate == 30) {
                                info.frame_property_bitmask |= (1 << VIDEO_320x240x30);
                            } else if (framerate == 60) {
                                info.frame_property_bitmask |= (1 << VIDEO_320x240x60);
                            }
                            break;
                        case FRAME_640x480:
                            if (framerate == 15) {
                                info.frame_property_bitmask |= (1 << VIDEO_640x480x15);
                            } else if (framerate == 30) {
                                info.frame_property_bitmask |= (1 << VIDEO_640x480x30);
                            } else if (framerate == 60) {
                                info.frame_property_bitmask |= (1 << VIDEO_640x480x60);
                            }
                            break;
                        case FRAME_1280x720:
                            if (framerate == 15) {
                                info.frame_property_bitmask |= (1 << VIDEO_1280x720x15);
                            } else if (framerate == 30) {
                                info.frame_property_bitmask |= (1 << VIDEO_1280x720x30);
                            } else if (framerate == 60) {
                                info.frame_property_bitmask |= (1 << VIDEO_1280x720x60);
                            }
                            break;
                        default:
                            ;
                        }
                    }

                // The PiCam doesn't list the resolutions explicitly, so we have
                // to guess its capabilities
                } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                    // How do I get the framerates for stepwise cams?
                    info.frame_property_bitmask |= (1 << VIDEO_320x240x15);
                    info.frame_property_bitmask |= (1 << VIDEO_320x240x30);
                    info.frame_property_bitmask |= (1 << VIDEO_320x240x60);

                    info.frame_property_bitmask |= (1 << VIDEO_640x480x15);
                    info.frame_property_bitmask |= (1 << VIDEO_640x480x30);
                    info.frame_property_bitmask |= (1 << VIDEO_640x480x60);

                    info.frame_property_bitmask |= (1 << VIDEO_1280x720x15);
                    info.frame_property_bitmask |= (1 << VIDEO_1280x720x30);
                    info.frame_property_bitmask |= (1 << VIDEO_1280x720x60);
                    break;
                }
            }

            device_properties_map.insert(pair<string, v4l2_info>(dev, info));
            close(fd);
        }
        i++;
    }
}

void RTSPStreamServer::remove_mount_point(string mount_point)
{
    GstRTSPMountPoints* mounts;
    mounts = gst_rtsp_server_get_mount_points(server);
    gst_rtsp_mount_points_remove_factory(mounts, mount_point.c_str());
}

void RTSPStreamServer::remove_stream(string device)
{
    auto device_itr = device_properties_map.find(device);
    auto stream_itr = adaptive_streams_map.find(device);
    auto list_itr = find(device_list.begin(), device_list.end(), device);

    remove_mount_point(device_itr->second.mount_point);

    device_list.erase(list_itr);
    device_properties_map.erase(device_itr);
    adaptive_streams_map.erase(stream_itr);
}

void RTSPStreamServer::setup_streams()
{
    for (auto it = device_properties_map.begin(); it != device_properties_map.end(); it++) {
        adaptive_streams_map.insert(pair<string,RTSPAdaptiveStreaming*>(it->first,
                                    new RTSPAdaptiveStreaming(it->first,
                                            it->second.camera_type,
                                            it->second.mount_point, server)));
    }
}

map<string, RTSPAdaptiveStreaming*> RTSPStreamServer::get_stream_map()
{
    return adaptive_streams_map;
}

map<string, v4l2_info> RTSPStreamServer::get_device_map()
{
    return device_properties_map;
}

GstRTSPServer* RTSPStreamServer::get_server()
{
    return server;
}

string RTSPStreamServer::get_ip_address()
{
    return ip_addr;
}

string RTSPStreamServer::get_port()
{
    return port;
}

RTSPStreamServer* RTSPStreamServer::get_instance(string _ip_addr, string _port)
{
    if (!initialised) {
        instance = new RTSPStreamServer(_ip_addr, _port);
        initialised = true;
    }
    return instance;
}

RTSPStreamServer* RTSPStreamServer::get_instance()
{
    if (initialised) {
        return instance;
    }
    return nullptr;
}

RTSPStreamServer::~RTSPStreamServer()
{
    for (auto stream_pair : adaptive_streams_map) {
        delete stream_pair.second;
    }
    delete instance;
    initialised = false;
}