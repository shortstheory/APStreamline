2DO:

* what to do with UDPStreaming?
* UDPStreaming handle args better
* Write comments in headers

PARTIAL:
* Do APSync integration
* get framerates for stepwise cams

DONE:
* Work on UVCH264 cams eg C920
* fix the issue with callbacks execution order in file recorder
* struct QoSReport instead of class, looks pretty silly as a class
* Update README.md
* add scripts
* use static element names instead in rtspstreamer
* Get IP Addr of device
* Record on CC

SKIP:
* Add licenses
* what do I do with ntptime?
* Put V4L2 info fields into RTSPAdaptiveStreaming

NOTES:
x264enc gives huge buffers but they can't be used for any estimates

c920: resolutions can't be switched no matter what i do, using a capsfilter element doesn't work at all and defaults to a 1080p stream, so idk!?!

it's safe to unlink the pads on the cb of a blocking probe

c920 needs a queue for receiving streams: gst-launch-1.0 -v udpsrc caps="application/x-rtp" port=5000 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink

for sending: 

gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-h264, width=1280, height=720, framerate=30/1 ! queue ! rtph264pay ! udpsink port=5000

c920 is badly behaved, sends empty buffers which are detected as eos: https://bugzilla.gnome.org/show_bug.cgi?id=794842

it can behave as a sw adaptive encoder and a hardware single res encoder? not much point of swapping around bitrates for it