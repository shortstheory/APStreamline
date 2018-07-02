
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

    RTSPMessageType get_message_type(char* buf)
    {
        string buffer(buf);
        cout << "Recevied buffer - " << buf << endl;
        for (int i = 0; i < RTSPMessageHeader.size(); i++) {
            if (!buffer.compare(0, RTSPMessageHeader[i].size(), RTSPMessageHeader[i])) {
                cout << "Message type - " << RTSPMessageHeader[i] << " " << static_cast<RTSPMessageType>(i);
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

    string serialise_device_props(v4l2_info device_props)
    {
        string list;
        list = RTSPMessageHeader[RTSPMessageType::GET_DEVICE_PROPS] + "$";
        string dev_info;
        // weird hack to work around \0?
        // dev_info = device_props.camera_name.substr(0, device_props.camera_name.size()-1) + "!" + device_props.mount_point + "!" + to_string(device_props.camera_type);
        char info_buffer[100];
        // dev_info =  + "!" +  + "!" + to_string(device_props.camera_type);
        sprintf(info_buffer, "#%s!%s!%s\0", device_props.camera_name.c_str(), device_props.mount_point.c_str(), to_string(device_props.camera_type).c_str());
        // cout << dev_info;
        printf("SERIALIST!! %s", info_buffer);
        return string(info_buffer);
    }

    bool send_string(string data)
    {
        int numbytes;
        numbytes = send(client_fd, data.c_str(), data.length(), 0);
        if (numbytes > 0) {
            cout << "Bytes sent - " << numbytes << endl;
            return true;
        }
        return false;
    }

    void send_device_props()
    {
        vector<v4l2_info> device_props;
        device_props = rtsp_stream_server->get_device_properties();
        for (v4l2_info info : device_props) {
            string device_list;
            device_list = serialise_device_props(info);
            send_string(device_list);
        }
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
            break;
        case RTSPMessageType::TMP:
            break;
        case RTSPMessageType::ERR:
            g_warning("Unrecognised header");
            break;
        }
    }

};

#endif