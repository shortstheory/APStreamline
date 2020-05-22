project('adaptive-streaming', 'c', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
gst_rtsp_dep = dependency('gstreamer-rtsp-1.0')
gst_rtsp_server_dep = dependency('gstreamer-rtsp-server-1.0')
thread_dep = dependency('threads')

common_deps = ['src/Common/PipelineManager.cpp', 'src/Common/QoSEstimator.cpp']
rtsp_src = ['src/RTSPStreamServer/stream_server.cpp', 'src/RTSPStreamServer/IPCMessageHandler.cpp', 'src/RTSPStreamServer/RTSPAdaptiveStreaming.cpp', 'src/RTSPStreamServer/RTSPStreamServer.cpp']
executable('stream_server', sources: [rtsp_src, common_deps], dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep, thread_dep], cpp_args : '-Werror', install : true)
