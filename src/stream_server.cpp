#include "RTSPStreamServer.h"
#include <iostream>
#include <thread>

void ipc_loop(RTSPStreamServer* streamer)
{
    // while (true)
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