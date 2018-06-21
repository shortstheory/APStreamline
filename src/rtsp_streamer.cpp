#include "RTSPAdaptiveStreaming.h"

int main(int argc, char *argv[])
{
    gst_init (&argc, &argv);

    GMainLoop* loop = g_main_loop_new (NULL, FALSE);
    GstRTSPServer* server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554");
    gst_rtsp_server_set_address(server, "172.17.0.2");
    g_print ("custom Stream ready at rtsp://172.17.0.2:8554/test");
    RTSPAdaptiveStreaming rtsp_adaptive_streaming("/dev/video0", GenericAdaptiveStreaming::CameraType::RAW_CAM,
                                                "/test", server);
    gst_rtsp_server_attach (server, NULL);
    g_main_loop_run (loop);

    return 0;
}