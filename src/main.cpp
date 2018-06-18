#include "AdaptiveStreaming.h"

int main(int argc, char *argv[])
{
    string receiver_ip_addr;
    string dev;
    // receiver_ip_addr = "192.168.0.102";
    // receiver_ip_addr = "10.42.0.56";
    // receiver_ip_addr = "192.168.1.2";
    // g_warning("%d", argc);
    if (argc == 1) {
        dev = "/dev/video0";
        receiver_ip_addr = "127.0.0.1";
    } else {
        dev = argv[1];
        receiver_ip_addr = argv[2];
    }

    g_warning("Sending feed from %s to %s", dev.c_str(), receiver_ip_addr.c_str());

    gst_init(&argc, &argv);
    AdaptiveStreaming adaptiveStreaming(dev, receiver_ip_addr, AdaptiveStreaming::CameraType::RAW_CAM, 5000, 5001);
    adaptiveStreaming.start_playing();
    GstBus* bus = adaptiveStreaming.get_pipeline_bus();
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // g_warning("done");
    return 0;
} 
