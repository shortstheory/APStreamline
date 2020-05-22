#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <gst/gst.h>
#include <vector>

using namespace std;

// This generalisation is hardly rigid, but we can target higher bitrates on
// x86/amd64 systems which are typically laptops. ARM devices such as the
// Raspberry Pi are far more limited.

#ifdef __amd64__
static const guint32 MAX_STEADY_BITRATE = 4000;
#endif

#ifdef __aarch64__
static const guint32 MAX_STEADY_BITRATE = 4000;
#endif

#ifdef __arm__
static const guint32 MAX_STEADY_BITRATE = 4000;
#endif

static const guint32 MIN_STEADY_BITRATE = 500;
static const guint32 INC_STEADY_BITRATE = 500;
static const guint32 DEC_STEADY_BITRATE = 1000;

static const guint32 MAX_CONGESTION_BITRATE = 1000;
static const guint32 MIN_CONGESTION_BITRATE = 100;
static const guint32 INC_CONGESTION_BITRATE = 250;
static const guint32 DEC_CONGESTION_BITRATE = 500;

static const guint32 LOW_QUAL_BITRATE = 500;
static const guint32 MED_QUAL_BITRATE = 1000;
static const guint32 HIGH_QUAL_BITRATE = 3000;

static const guint32 IPC_BUFFER_SIZE = 10000;

static const guint32 I_FRAME_INTERVAL = 10;
static const guint32 SUCCESSFUL_TRANSMISSION = 5;

static const vector<string> MJPG_CAPS_FILTERS = {
    "image/jpeg,width=(int)320,height=(int)240,framerate=(fraction)15/1",
    "image/jpeg,width=(int)640,height=(int)480,framerate=(fraction)15/1",
    "image/jpeg,width=(int)1280,height=(int)720,framerate=(fraction)15/1",
    "image/jpeg,width=(int)320,height=(int)240,framerate=(fraction)30/1",
    "image/jpeg,width=(int)640,height=(int)480,framerate=(fraction)30/1",
    "image/jpeg,width=(int)1280,height=(int)720,framerate=(fraction)30/1",
    "image/jpeg,width=(int)320,height=(int)240,framerate=(fraction)60/1",
    "image/jpeg,width=(int)640,height=(int)480,framerate=(fraction)60/1",
    "image/jpeg,width=(int)1280,height=(int)720,framerate=(fraction)60/1"
};

static const vector<string> H264_CAPS_FILTERS = {
    "video/x-h264,width=(int)320,height=(int)240,framerate=(fraction)15/1",
    "video/x-h264,width=(int)640,height=(int)480,framerate=(fraction)15/1",
    "video/x-h264,width=(int)1280,height=(int)720,framerate=(fraction)15/1",
    "video/x-h264,width=(int)320,height=(int)240,framerate=(fraction)30/1",
    "video/x-h264,width=(int)640,height=(int)480,framerate=(fraction)30/1",
    "video/x-h264,width=(int)1280,height=(int)720,framerate=(fraction)30/1",
    "video/x-h264,width=(int)320,height=(int)240,framerate=(fraction)60/1",
    "video/x-h264,width=(int)640,height=(int)480,framerate=(fraction)60/1",
    "video/x-h264,width=(int)1280,height=(int)720,framerate=(fraction)60/1"
};

static const vector<string> JETSON_CAPS_FILTERS = {
    "video/x-raw(memory:NVMM),width=(int)320,height=(int)240,format=(string)NV12,framerate=(fraction)15/1",
    "video/x-raw(memory:NVMM),width=(int)640,height=(int)480,format=(string)NV12,framerate=(fraction)15/1",
    "video/x-raw(memory:NVMM),width=(int)1280,height=(int)720,format=(string)NV12,framerate=(fraction)15/1",
    "video/x-raw(memory:NVMM),width=(int)320,height=(int)240,format=(string)NV12,framerate=(fraction)30/1",
    "video/x-raw(memory:NVMM),width=(int)640,height=(int)480,format=(string)NV12,framerate=(fraction)30/1",
    "video/x-raw(memory:NVMM),width=(int)1280,height=(int)720,format=(string)NV12,framerate=(fraction)30/1",
    "video/x-raw(memory:NVMM),width=(int)320,height=(int)240,format=(string)NV12,framerate=(fraction)60/1",
    "video/x-raw(memory:NVMM),width=(int)640,height=(int)480,format=(string)NV12,framerate=(fraction)60/1",
    "video/x-raw(memory:NVMM),width=(int)1280,height=(int)720,format=(string)NV12,framerate=(fraction)60/1"
};

static const int AUTO_PRESET = 1024;
static const string SOCKET_PATH = "/tmp/rtsp_server.sock";

static const string V4L2_DEVICE_PATH = "/dev/";
static const string V4L2_DEVICE_PREFIX = "video";
static const string MOUNT_POINT_PREFIX = "/cam";
static const string JETSON_CAM_DRIVER = "tegra-video";


#endif