# Adaptive H.264 Streaming Using GStreamer

## Introduction

Using video cameras for live-streaming the video feed from quadcopters and other unmanned vehicles is becoming increasingly useful. Most video streaming solutions use RTP for streaming video over UDP. UDP is more efficient than TCP because it forgoes the reliable delivery and congestion control mechanisms that TCP boasts.

However, this introduces new problems when streaming video from robots. In most cases, we use the Companion Computer (CC) in Wi-Fi hotspot mode for streaming the video. This means that the video Quality-of-Service progressively gets worse when the robot goes further away from the receiving computer.

The Adaptive Streaming aims to fix this problem by dynamically adjusting the video quality by parsing the RTCP Receiver Report packets on the CC. These RTCP packets provide helpful QoS information which can be used for automatically changing the bitrate and resolution of the video delivered from the CC.

Currently this project supports H.264 hardware encoding the video feed of the Raspberry Pi Camera on the Raspberry Pi 3B(+). H.264 software encoding is supported for all other V4L2 cameras.

### Note for hardware encoding webcams

Some webcams such as the Logitech C920 support hardware encoding through an onboard processor, however bugs in the UVC driver has resulted in some reduced functionality. These webcams are configured to adaptively stream in only 480p through adjusting the H.264 video bitrate. Switching the resolution results in the video client closing due to the transmission of an EOS event once the resolution has changed.

## Running the Code

Install the `gstreamer` dependencies:

```
sudo apt install libgstreamer-plugins-base1.0* libgstreamer1.0-dev libgstrtspserver-1.0-dev gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly python3-pip
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

By default, port 5000 is used for sending RTP packets and port 5001 is used for sending and receiving RTCP packets.

On the Raspberry Pi, use `sudo modprobe bcm2835-v4l2` to load the V4L2 driver for the Raspberry Pi camera. Add `bcm2835-v4l2` to `/etc/modules` for automatically loading this module on boot.

## Usage

### RTSP Streaming

### UDP Streaming

*Use this if you only need to stream from one camera at a time or your GCS doesn't support RTSP streaming*

On installing `adaptive-streaming`, run it from the terminal as so:

Software encoding - `adaptive_streaming /dev/video0 <RECEIVER_IP> raw`

Hardware encoding (recommended for the Raspberry Pi) - `adaptive_streaming /dev/video0 <RECEIVER_IP> h264`

For receiving the video stream, use the following `gst-launch` pipeline on the receiver:

`gst-launch-1.0 -v rtpbin latency=0 name=rtpbin udpsrc caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264,payload=96" port=5000 !  rtpbin.recv_rtp_sink_0 rtpbin. ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink udpsrc port=5001 ! rtpbin.recv_rtcp_sink_0 rtpbin.send_rtcp_src_0 ! udpsink port=5001 sync=false async=false host=<SENDER_IP>`

For APSync images with the CC on 10.0.1.128, you can directly run `./recv_apsync_video`.

Typically, this `gst-launch` command should be run on the receiver before streaming the video from the companion computer.

## Troubleshooting

Sometimes you might see:

```
** (adaptive_streaming:1358): CRITICAL **: gst_rtcp_packet_get_rb: assertion 'nth < packet->count' failed
```

printed on STDOUT when running `adaptive-streaming`. In this case, please check the `<SENDER_IP>` and  `<RECEIVER_IP>` in the above commands.