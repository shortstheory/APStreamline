
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

class IPCMessageHandler
{
private:
    int client_fd;
    RTSPStreamServer* rtsp_stream_server;

    RTSPMessageType get_message_type(char* buf);
    string get_message_payload(char* buf);
    string serialise_device_props(pair<string, v4l2_info> device_props);
    bool send_string(string data);
    void send_device_props();
    void set_device_quality(char* buffer);

public:
    IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server);
    void process_msg(char* buf);
};

#endif