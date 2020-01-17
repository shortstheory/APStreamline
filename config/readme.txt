To use an external configuration, make a file containing the relevant pipeline:

MJPEG: mjpg_cam.conf
UVC: uvc_cam.conf
h.264: h264_cam.conf
Jetson: jetson_cam.conf

Put the file in the bin/ directory where the stream_server executable is, and 
launch stream_server. If present, it will use the configuration file.

