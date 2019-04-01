# APStreamline

*Adaptive H.264 Streaming From ArduPilot Robots*

## Introduction

Using video cameras for live-streaming the video feed from aerial robots and other unmanned vehicles is becoming increasingly useful. Most video streaming solutions use RTP for streaming video over UDP. UDP is more efficient than TCP because it forgoes the overhead that comes with TCP's reliable delivery and congestion control mechanisms.

However, this introduces new problems when streaming video from robots. In most cases, we use the Companion Computer (CC) in Wi-Fi hotspot mode for streaming the video. Due to the limited range of 2.4GHz Wi-Fi, the Quality-of-Service (QoS) progressively gets worse when the robot moves further away from the receiving computer.

The APStreamline project aims to fix this problem by dynamically adjusting the video quality. Over UDP we can obtain estimates of QoS using RTCP packets received from the receiver. These RTCP packets provide helpful QoS information (such as RTT and packet loss) which can be used for automatically changing the bitrate and resolution of the video delivered from the sender.

The code makes use of GStreamer libraries for creating the streaming pipelines.

### Recent Changes

APStreamline was first released in a beta image of APSync in [September 2018](https://discuss.ardupilot.org/t/apsync-with-apstreamline-beta/33247). The latest release supports the following features and needs to built from source:

* ***NEW***: Support for using the hardware encoder for Jetson TX1/TX2 CSI cameras

* ***NEW***: More flexiblity for UVC/CSI cameras with more options to change the camera's resolution and frame-rate

* **Automatic** quality selection based on **bandwidth** and **packet loss** estimates 

* **Selection** of network interfaces to stream the video 

* <s>Options to **record** the live-streamed video feed to the companion computer</s>. This had to be removed because of stability issues.

* **Manual control** over resolution and framerates 

* **Multiple** camera support using RTSP 

* **Hardware-accelerated** encoding for the **Raspberry Pi** camera on the Raspberry Pi

* Camera settings **configurable** through the **APWeb GUI**

## Running the Code

### Hardware

All the following instructions are for installing APStreamline and APWeb on the CC. A Raspberry Pi 2/3/3B+ with the latest version of Raspian or APSync is a good choice. Intel NUC's are good choices as well. As of April 2019, APStreamline also provides support for the Nvidia Jetson TX1/TX2 boards and its CSI cameras with the `tegra-video` drivers. 

Do note that the Raspberry Pi 3 and 3B+ have **very** low power Wi-Fi antennae which aren't great for video streaming. Using a portable Wi-Fi router like the TPLink MR3020 can dramatically improve range. Wi-Fi USB dongles working in hotspot mode can help as well.

### Installing APStreamline

Install the `gstreamer` dependencies:

```
sudo apt install libgstreamer-plugins-base1.0* libgstreamer1.0-dev libgstrtspserver-1.0-dev gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly python3-pip python-pip
```

Install `meson` from `pip` and `ninja` for building the code:

```
sudo pip3 install meson
sudo pip install pymavlink
sudo apt install ninja-build
```

Navigate to the cloned folder folder and run:

```
meson build
cd build
meson configure -Dprefix=$HOME/start_apstreamline/
ninja install # installs to ~/start_apstreamline for APWeb to spawn the process
```

On the Raspberry Pi, use `sudo modprobe bcm2835-v4l2` to load the V4L2 driver for the Raspberry Pi camera. Add `bcm2835-v4l2` to `/etc/modules` for automatically loading this module on boot.

### Installing APWeb

The [APWeb server](https://github.com/shortstheory/APWeb) project enables setting several flight controller parameters on the fly through the use of a Companion Computer (e.g. the Raspberry Pi). We use this APWeb server for configuring the video streams as well.

Clone the forked branch with APStreamline support here:

```
git clone -b video_streaming https://github.com/shortstheory/APWeb.git
cd APWeb
```

Install `libtalloc-dev` and get the MAVLink submodule:

```
sudo apt-get install libtalloc-dev
git submodule update --init --recursive
```

Build APWeb:

```
cd APWeb
make
sudo ./web_server -p 80
```

## Usage

Video livestreams can be launched using RTSP. It is recommended to use RTSP for streaming video as it provides the advantages of supporting multiple cameras, conifguring the resolution on-the-fly, and recording the livestreamed video to a file.

### RTSP Streaming

#### APWeb

Start the APWeb server. This will serve the configuration page for the RTSP stream server. Connect to the web server in your favourite web browser by going to the IP address of the Companion Computer.

On navigating to the new `video/` page, you will be presented with a page to start the RTSP Server:

![Screenshot](screenshots/server_stop.png)

On selecting the desired interface and starting the RTSP Server, the APWeb server will spawn the Stream Server process. The stream server will search for all the V4L2 cameras available in `/dev/`. It will query the capabilities of all these cameras and select hardware encoding or software encoding accordingly. The list of available cameras can be refreshed by simply stopping and starting the server.

From here, the APWeb page will display the list of available RTSP streams and their mount points:

![Screenshot](screenshots/server_start.png)

The video quality can either be automatically set based on the avaialble network bandwidth or set manually for more fine-grained control.

The RTSP streams can be viewed using any RTSP player. VLC is a good choice.

For example, this can be done in VLC by going to "Media > Open Network Stream" and pasting in the RTSP Mount Point for the camera displayed in the APWeb configuration page. However, VLC introduces *two* seconds of latency for the jitter reduction, making it unsuitable for critical applications. To circumvent this, RTSP streams can also be viewed at lower latency by using the `gst-launch` command:

`gst-launch-1.0 playbin uri=<RTSP-MOUNT-POINT> latency=100`

An RTSP Mount Point looks like this: `rtsp://192.168.0.17:8554/cam0`. Refer to the APWeb page to see the mount points given for your camera.

#### Standalone

Launch the RTSP stream server by running:

`./stream_server <interface>`

The list of available network interfaces can be found by running `ifconfig`.
