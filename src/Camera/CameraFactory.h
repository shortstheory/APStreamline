#ifndef CAMERA_FACTORY_H
#define CAMERA_FACTORY_H

#include "../Common/DeviceDatatypes.h"
#include "../Camera/Camera.h"
#include "../Camera/MJPGCamera.h"
#include <memory>
using namespace std;
class CameraFactory
{
private:
    shared_ptr<Camera> camera;
public:
    CameraFactory(string device, Quality q, CameraType type)
    {
        if (type == MJPG_CAM) { 
            camera = shared_ptr<MJPGCamera>(new MJPGCamera(device,q));
        }
    }
    shared_ptr<Camera> get_camera()
    {
        return camera;
    }
};

#endif