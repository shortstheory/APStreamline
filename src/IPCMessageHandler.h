
#ifndef IPC_MESSAGE_HANDLER_H
#define IPC_MESSAGE_HANDLER_H

#include "RTSPStreamServer.h"

#include <iostream>
// #include <stdlib.h>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

class IPCMessageHandler {
private:
    int client_fd;
    RTSPStreamServer* rtsp_stream_server;

    RTSPMessageType get_message_type(char* buf);
    string get_message_payload(char* buf);
    string serialise_device_props(v4l2_info device_props);
    bool send_string(string data);
    void send_device_props();

public:
    IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server);
    void process_msg(char* buf);
};

#endif