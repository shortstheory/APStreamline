# Adaptive H.264 Streaming Using GStreamer

## Introduction

Using video cameras for live-streaming the video feed from quadcopters and other unmanned vehicles is becoming increasingly useful. Most video streaming solutions use RTP for streaming video over UDP. UDP is more efficient than TCP because it forgoes the reliable delivery and congestion control mechanisms that TCP boasts.

However, this introduces new problems when streaming video from robots. 

## Building the code

First install the `gstreamer` dependencies. The list will be updated here soon.

```
meson build
cd build
ninja
./adaptive_streaming
```