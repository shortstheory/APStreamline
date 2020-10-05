#ifndef CAMERA_FACTORY_H
#define CAMERA_FACTORY_H

#include "../Common/DeviceDatatypes.h"
#include "../Camera/Camera.h"
#include "../Camera/MJPGCamera.h"
#include "../Camera/C920Camera.h"
#include "../Camera/ZEDCamera.h"
#include "../Camera/AR0521Camera.h"
#include <memory>

using namespace std;

class CameraFactory
{
private:
    shared_ptr<Camera> camera;
public:
    CameraFactory(string device, Quality q, CameraType type)
    {
        if (type == CameraType::MJPG_CAM) {
            camera = shared_ptr<MJPGCamera>(new MJPGCamera(device,q));
        } else if (type == CameraType::C920_CAM) {
            camera = shared_ptr<C920Camera>(new C920Camera(device,q));
        } else if (type == CameraType::ZED_CAM) {
            camera = shared_ptr<ZEDCamera>(new ZEDCamera(device,q));
        } else if (type == CameraType::AR0521_CAM) {
            camera = shared_ptr<AR0521Camera>(new AR0521Camera(device,q));
        }
    }

    shared_ptr<Camera> get_camera()
    {
        return camera;
    }
};

#endif
