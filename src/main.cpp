#include "UDPAdaptiveStreaming.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    string receiver_ip_addr;
    string dev;
    string cam_type;
    // receiver_ip_addr = "192.168.0.102";
    // receiver_ip_addr = "10.42.0.56";
    // receiver_ip_addr = "192.168.1.2";
    // g_warning("%d", argc);
    if (argc == 1) {
        dev = "/dev/video0";
        receiver_ip_addr = "127.0.0.1";
        cam_type = "raw";
    }
    else {
        dev = argv[1];
        receiver_ip_addr = argv[2];
        cam_type = argv[3];
    }

    g_warning("Sending feed from %s to %s", dev.c_str(), receiver_ip_addr.c_str());

    gst_init(&argc, &argv);
    // UDPAdaptiveStreaming adaptiveStreaming(dev, GenericAdaptiveStreaming::CameraType::RAW_CAM, receiver_ip_addr);
    // adaptiveStreaming.play_pipeline();
    // GstBus* bus = adaptiveStreaming.get_pipeline_bus();
    UDPAdaptiveStreaming* adaptiveStreaming;
    if (cam_type == "h264") {
        adaptiveStreaming = new UDPAdaptiveStreaming(dev, GenericAdaptiveStreaming::CameraType::H264_CAM, receiver_ip_addr);
    }
    else if (cam_type == "raw") {
        adaptiveStreaming = new UDPAdaptiveStreaming(dev, GenericAdaptiveStreaming::CameraType::RAW_CAM, receiver_ip_addr);
    }
    else {
        g_warning("Camera type not recognised - use `raw` or `h264`");
    }
    adaptiveStreaming->play_pipeline();
    GstBus* bus = adaptiveStreaming->get_pipeline_bus();
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    delete adaptiveStreaming;
    // g_warning("done");
    return 0;
}
