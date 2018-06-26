#include "RTSPStreamServer.h"
#include <sys/ioctl.h>
#include <linux/videodev.h>

bool RTSPStreamServer::initialised = true;
RTSPStreamServer* RTSPStreamServer::instance = nullptr;

RTSPStreamServer::RTSPStreamServer()
{

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
}