#ifndef DEVICE_DATATYPES_H
#define DEVICE_DATATYPES_H

#include <string>

using namespace std;

enum CameraType {RAW_CAM, UVC_CAM, H264_CAM, MANUAL_CAM};
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

struct v4l2_info {
    string camera_name;
    string mount_point;
    CameraType camera_type;
    guint64 frame_property_bitmask;
};

#endif