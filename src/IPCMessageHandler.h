
#ifndef IPC_MESSAGE_HANDLER_H
#define IPC_MESSAGE_HANDLER_H

#include "RTSPStreamServer.h"
#include <iostream>
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

    RTSPMessageType get_message_type(char* buf)
    {
        string buffer(buf);

        for (int i = 0; i < 2; i++) {
            if (buffer.compare(0, RTSPMessageHeader[i].size(), RTSPMessageHeader[i])) {
                return static_cast<RTSPMessageType>(i);
            }
        }
        return RTSPMessageType::ERR;
    }

    string get_message_payload(char* buf)
    {
        string buffer(buf);
        return buffer.substr(3);
    }

    string serialise_device_props(vector<v4l2_info> device_props)
    {
        string list;
        for (auto it = device_props.begin(); it != device_props.end(); it++) {
            string dev_info;
            dev_info = it->camera_name + "::" + it->mount_point + "::" + to_string(it->camera_type);
            list = list + "||" + dev_info;
        }
        return list;
    }

    bool send_string(string data)
    {
        int numbytes;
        numbytes = send(client_fd, data.c_str(), data.length(), 0);
        if (numbytes > 0) {
            return true;
        }
        return false;
    }

    bool send_device_props()
    {
        string device_list;
        vector<v4l2_info> device_props;
        device_list = serialise_device_props(device_props);
        return send_string(device_list);
    }

public:
    IPCMessageHandler(int fd, RTSPStreamServer* _rtsp_stream_server) : client_fd(fd)
    {
        assert(_rtsp_stream_server);
        assert(client_fd != -1);
        rtsp_stream_server = _rtsp_stream_server;
    }

    void process_msg(char* buf)
    {
        RTSPMessageType msgtype;
        msgtype = get_message_type(buf);
        switch (msgtype) {
        case RTSPMessageType::GET_DEVICE_PROPS:
            send_device_props();
        case RTSPMessageType::ERR:
            g_warning("Unrecognised header");
        }
    }

};

#endif