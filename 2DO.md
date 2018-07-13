2DO:

* add scripts
* Work on RAW/H264 cams eg C920
* Write comments in headers
* Update README.md
* Do APSync integration
* UDPStreaming handle args better

PARTIAL:
* get framerates for stepwise cams
* Record on CC

DONE:
* Get IP Addr of device

SKIP:
* Put V4L2 info fields into RTSPAdaptiveStreaming

NOTES:
c920 needs a queue for receiving streams: gst-launch-1.0 -v udpsrc caps="application/x-rtp" port=5000 ! queue ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink

for sending: 

gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-h264, width=1280, height=720, framerate=30/1 ! queue ! rtph264pay ! udpsink port=5000