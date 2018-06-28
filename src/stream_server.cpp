#include "RTSPStreamServer.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char socket_path[80] = "mysocket";


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

// bool send_device_props(vector<v4l2_info> device_props)
// {
//     // string
//     string device_list;
//     device_list = serialise_device_props(device_props);
//     return send_string(device)
// }

bool send_string(string data, int cli_fd)
{
    int numbytes;
    numbytes = send(cli_fd, data.c_str(), data.length(), 0);
    if (numbytes > 0) {
        return true;
    }
    return false;
}

void ipc_loop(RTSPStreamServer* streamer)
{
    struct sockaddr_un addr;
    char buf[100];
    int socket_fd, client_fd, bytes_read;

    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        g_warning("Local socket creation failed, IPC will be disabled");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);
    unlink(socket_path);

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        g_warning("Bind error");
        return;
    }

    if (listen(socket_fd, 1) == -1) {
        g_warning("Listen error");
        return;
    }

    while (true) {
        if ((client_fd = accept(socket_fd, NULL, NULL)) == -1) {
            g_warning("Connection failed");
            continue;
        }
        g_warning("Connection accepted!");
        while ((bytes_read=read(client_fd,buf,sizeof(buf))) > 0) {
            printf("read %u bytes: %s\n", bytes_read, buf);
        }
        if (bytes_read == -1) {
        }
    }
}

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    RTSPStreamServer* rtsp_stream_server = RTSPStreamServer::get_instance();
    gst_rtsp_server_attach(rtsp_stream_server->get_server(), NULL);

    thread t(&ipc_loop, rtsp_stream_server);
    t.detach();

    g_main_loop_run(loop);
    return 0;
}