project('adaptive-streaming', 'c', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
gst_rtsp_dep = dependency('gstreamer-rtsp-1.0')
gst_rtsp_server_dep = dependency('gstreamer-rtsp-server-1.0')
thread_dep = dependency('threads')

src = ['src/main.cpp', 'src/FileRecorder.cpp', 'src/GenericAdaptiveStreaming.cpp', 'src/QoSEstimator.cpp', 'src/UDPAdaptiveStreaming.cpp', 'src/RTSPAdaptiveStreaming.cpp']
ss_src = ['src/stream_server.cpp', 'src/FileRecorder.cpp', 'src/IPCMessageHandler.cpp', 'src/GenericAdaptiveStreaming.cpp', 'src/QoSEstimator.cpp', 'src/UDPAdaptiveStreaming.cpp', 'src/RTSPAdaptiveStreaming.cpp', 'src/RTSPStreamServer.cpp']

executable('adaptive_streaming', sources: src, dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep], cpp_args : '-std=c++14', install : true)
executable('stream_server', sources: ss_src, dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep, thread_dep], cpp_args : '-std=c++14', install : true)
