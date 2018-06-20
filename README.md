# Adaptive H.264 Streaming Using GStreamer

## Introduction

Using video cameras for live-streaming the video feed from quadcopters and other unmanned vehicles is becoming increasingly useful. Most video streaming solutions use RTP for streaming video over UDP. UDP is more efficient than TCP because it forgoes the reliable delivery and congestion control mechanisms that TCP boasts.

However, this introduces new problems when streaming video from robots. In most cases, we use the Companion Computer (CC) in Wi-Fi hotspot mode for streaming the video. This means that the video Quality-of-Service progressively gets worse when the robot goes further away from the receiving computer.

The Adaptive Streaming aims to fix this problem by dynamically adjusting the video quality by parsing the RTCP Receiver Report packets on the CC. These RTCP packets provide helpful QoS information which can be used for automatically changing the bitrate and resolution of the video delivered from the CC.

Currently this project supports H.264 hardware encoding the video feed of the Raspberry Pi Camera on the Raspberry Pi 3B(+). H.264 software encoding is supported for all other V4L2 cameras.

Support for NVidia Jetson boards will be added soon.

## Running the Code

Install the `gstreamer` dependencies:

```
sudo apt install libgstreamer-plugins-base1.0* libgstreamer1.0-dev libgstrtspserver-1.0-dev
```

Install `meson` from `pip` and `ninja` for building the code:

```
sudo pip3 install meson
sudo apt install ninja-build
```

Navigate to the `adaptive-streaming` folder and run:

```
meson build
cd build
sudo ninja install
adaptive_streaming
```