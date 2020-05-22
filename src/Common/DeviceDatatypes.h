#ifndef DEVICE_DATATYPES_H
#define DEVICE_DATATYPES_H

#include <string>
#include <vector>
#include <gst/gst.h>
#include "Constants.h"

using namespace std;

enum CameraType {MJPG_CAM, UVC_CAM, RPI_CAM, JETSON_CAM};
enum ResolutionPresets {LOW, MED, HIGH};
enum FramePresets {FRAME_320x240, FRAME_640x480, FRAME_1280x720, FRAME_COUNT};
enum VideoPresets {VIDEO_320x240x15, VIDEO_640x480x15, VIDEO_1280x720x15,
                   VIDEO_320x240x30, VIDEO_640x480x30, VIDEO_1280x720x30,
                   VIDEO_320x240x60, VIDEO_640x480x60, VIDEO_1280x720x60
                  };

enum RTSPMessageType {GET_DEVICE_PROPS, SET_DEVICE_PROPS, ERROR, COUNT};
const static vector<string> RTSPMessageHeader = {
    "GDP", "SDP"
};


// Includes some metadata about each camera
struct v4l2_info {
    string camera_name;
    string mount_point;
    CameraType camera_type;
    guint64 frame_property_bitmask;
};

// Gives QoS estimates to the Adaptive Streamer for acting upon
struct QoSReport {
    guint8 fraction_lost;
    gfloat estimated_bitrate;
    gfloat encoding_bitrate;
    gfloat rtt;
    gfloat buffer_occ;

    QoSReport()
    {
    }

    QoSReport(guint8 _fl, gfloat _estd_br, gfloat _enc_br, gfloat _rtt, gfloat bo) :
        fraction_lost(_fl), estimated_bitrate(_estd_br), encoding_bitrate(_enc_br),
        rtt(_rtt), buffer_occ(bo)
    {
    }
};


#endif