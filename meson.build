project('adaptive-streaming', 'c', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
gst_rtsp_dep = dependency('gstreamer-rtsp-1.0')
gst_rtsp_server_dep = dependency('gstreamer-rtsp-server-1.0')
thread_dep = dependency('threads')

common_deps = ['src/Common/GenericAdaptiveStreaming.cpp', 'src/Common/QoSEstimator.cpp']
udp_src = ['src/UDP_Streaming/main.cpp', 'src/UDP_Streaming/UDPAdaptiveStreaming.cpp']
rtsp_src = ['src/RTSP_Stream_Server/stream_server.cpp', 'src/RTSP_Stream_Server/FileRecorder.cpp', 'src/RTSP_Stream_Server/IPCMessageHandler.cpp', 'src/RTSP_Stream_Server/RTSPAdaptiveStreaming.cpp', 'src/RTSP_Stream_Server/RTSPStreamServer.cpp']

executable('adaptive_streaming', sources: [udp_src, common_deps], dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep], cpp_args : '-std=c++14', install : true)
executable('stream_server', sources: [rtsp_src, common_deps], dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep, thread_dep], cpp_args : '-std=c++14', install : true)
