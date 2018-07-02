#ifndef DEVICE_DATATYPES_H
#define DEVICE_DATATYPES_H

#include <string>

using namespace std;

enum CameraType {RAW_CAM, H264_CAM};
enum ResolutionPresets {LOW, MED, HIGH};
enum RTSPMessageType {GET_DEVICE_PROPS, TMP, ERR, COUNT};

const static vector<string> RTSPMessageHeader = {
    "GDP", "TMP"
};

struct v4l2_info {
    string camera_name;
    string mount_point;
    CameraType camera_type;
};

#endif