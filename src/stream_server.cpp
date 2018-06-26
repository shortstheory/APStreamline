#include "RTSPStreamServer.h"


int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    RTSPStreamServer* rtsp_streamer = RTSPStreamServer::get_instance();
    gst_rtsp_server_attach(rtsp_streamer->get_server(), NULL);
    g_main_loop_run(loop);

    return 0;
}