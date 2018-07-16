2DO:

* remove void IPCMessageHandler::set_file_recording(char* buffer)
* use static element names instead in rtspstreamer
* add scripts
* Work on RAW/H264 cams eg C920
* Write comments in headers
* Update README.md
* Do APSync integration
* UDPStreaming handle args better
* Add licenses

PARTIAL:
* get framerates for stepwise cams

DONE:
* Get IP Addr of device
* Record on CC

SKIP:
* Put V4L2 info fields into RTSPAdaptiveStreaming

NOTES:
it's safe to unlink the pads on the cb of a blocking probe

c920 needs a queue for receiving streams: gst-launch-1.0 -v udpsrc caps="application/x-rtp" port=5000 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink

for sending: 

gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-h264, width=1280, height=720, framerate=30/1 ! queue ! rtph264pay ! udpsink port=5000

c920 is badly behaved, sends empty buffers which are detected as eos: https://bugzilla.gnome.org/show_bug.cgi?id=794842

it can behave as a sw adaptive encoder and a hardware single res encoder? not much point of swapping around bitrates for it