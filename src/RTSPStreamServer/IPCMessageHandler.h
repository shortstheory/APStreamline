
#ifndef IPC_MESSAGE_HANDLER_H
#define IPC_MESSAGE_HANDLER_H

#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "RTSPStreamServer.h"

using namespace std;

// Manages the Inter-Process Communication with the APWeb server code for sending
// the list of cameras available. Also triggers recording of the video and changing
// the quality of the video.

class IPCMessageHandler
{
private:
    int client_fd;

    // Requires a reference to the RTSP Stream Server for accessing the available streams
    RTSPStreamServer* rtsp_stream_server;

    string get_message_payload(char* buf);

    // Serialises the properties of a camera in JSON format for the APWeb server
    string serialise_device_props(pair<string, shared_ptr<RTSPAdaptiveStreaming>> device_props);
    bool send_string(string data);
    void set_device_quality(char* buffer);
    void send_device_props();

public:
    IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server);
    void process_msg(char* buf);
};

#endif