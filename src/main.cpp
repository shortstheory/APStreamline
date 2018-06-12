#include "AdaptiveStreaming.h"

int main(int argc, char *argv[])
{
    string receiver_ip_addr;
    receiver_ip_addr = "192.168.0.102";
    receiver_ip_addr = "127.0.0.1";
    receiver_ip_addr = "10.42.0.56";
    string dev = "/dev/video0";

    gst_init(&argc, &argv);
    AdaptiveStreaming adaptiveStreaming(dev, receiver_ip_addr, AdaptiveStreaming::CameraType::V4L2CAM);
    adaptiveStreaming.start_playing();
    GstBus* bus = adaptiveStreaming.get_pipeline_bus();
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // g_warning("done");
    return 0;
} 
