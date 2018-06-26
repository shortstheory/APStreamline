#ifndef RTSP_STREAM_SERVER_H
#define RTSP_STREAM_SERVER_H
#include "RTSPAdaptiveStreaming.h"

// Singleton class, we will never need more than one in a context

class RTSPStreamServer {
private:
    static bool initialised;
    static RTSPStreamServer* instance;
    RTSPStreamServer()
    {
        //private constructor
    }
public:
    static RTSPStreamServer* get_instance();
    ~RTSPStreamServer()
    {
        initialised = false;
    }
};

#endif
