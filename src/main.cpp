#include "AdaptiveStreaming.h"

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);
    AdaptiveStreaming adaptiveStreaming;
    adaptiveStreaming.start_playing();
    GstBus* bus = adaptiveStreaming.get_pipeline_bus();
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // g_warning("done");
    return 0;
} 
