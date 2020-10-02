project('adaptive-streaming', 'c', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
common_c_args = [
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_44',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_60',
  '-Wall'
]

gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
gst_rtsp_dep = dependency('gstreamer-rtsp-1.0')
gst_rtsp_server_dep = dependency('gstreamer-rtsp-server-1.0')
thread_dep = dependency('threads')
libconfig_dep = dependency('libconfig++')

common_deps = ['src/Common/PipelineManager.cpp', 'src/Common/QoSEstimator.cpp']
camera_deps = ['src/Camera/Camera.h','src/Camera/CameraFactory.h','src/Camera/Quality.cpp']
rtsp_src = ['src/RTSPStreamServer/stream_server.cpp', 'src/RTSPStreamServer/IPCMessageHandler.cpp', 'src/RTSPStreamServer/RTSPAdaptiveStreaming.cpp', 'src/RTSPStreamServer/RTSPStreamServer.cpp']
work_src = ['src/RTSPStreamServer/stream_server.cpp', 'src/Camera/C920Camera.cpp', 'src/Camera/ZEDCamera.cpp', 'src/Camera/MJPGCamera.cpp', 'src/Camera/Camera.cpp', 'src/RTSPStreamServer/RTSPAdaptiveStreaming.cpp', 'src/RTSPStreamServer/RTSPStreamServer.cpp']
executable('stream_server', sources: [work_src, common_deps, camera_deps], dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep, thread_dep, libconfig_dep], cpp_args : common_c_args, link_args: '-lstdc++fs', install : true)
