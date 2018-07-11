#include "RTSPStreamServer.h"
#include "IPCMessageHandler.h"

#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// char socket_path[80] = "/tmp/rtsp_server";
char socket_path[80] = "../../rtsp_server";

void ipc_loop(RTSPStreamServer* streamer)
{
    struct sockaddr_un addr;
    char buf[1000];
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

    // if ((client_fd = accept(socket_fd, NULL, NULL)) == -1) {
    //     g_warning("Connection failed");

    while (client_fd = accept(socket_fd, NULL, NULL)) {
        IPCMessageHandler message_handler(client_fd, streamer);
        g_warning("Connection accepted!");
        while ((bytes_read=read(client_fd,buf,sizeof(buf))) > 0) {
            buf[bytes_read] = '\0';
            message_handler.process_msg(buf);
            printf("read %u bytes: %s\n", bytes_read, buf);
        }
        g_warning("Disconnected from loop!");
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