#ifndef DEVICE_DATATYPES_H
#define DEVICE_DATATYPES_H

#include <string>

enum CameraType {RAW_CAM, H264_CAM};
enum ResolutionPresets {LOW, MED, HIGH};

using namespace std;

struct v4l2_info {
    string camera_name;
    string mount_point;
    CameraType camera_type;
};

#endif