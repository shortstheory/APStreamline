project('adaptive-streaming', 'c', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
gst_rtsp_dep = dependency('gstreamer-rtsp-1.0')
gst_rtsp_server_dep = dependency('gstreamer-rtsp-server-1.0')
src = ['src/main.cpp', 'src/GenericAdaptiveStreaming.cpp', 'src/QoSEstimator.cpp', 'src/UDPAdaptiveStreaming.cpp', 'src/RTSPAdaptiveStreaming.cpp']
rtsp_src = ['src/rtsp_streamer.cpp', 'src/GenericAdaptiveStreaming.cpp', 'src/QoSEstimator.cpp', 'src/UDPAdaptiveStreaming.cpp', 'src/RTSPAdaptiveStreaming.cpp']
executable('adaptive_streaming', sources: src, dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep], cpp_args : '-std=c++14', install : true)
executable('rtsp_streamer', sources: rtsp_src, dependencies: [gst_dep, gst_rtp_dep, gst_rtsp_dep, gst_rtsp_server_dep], cpp_args : '-std=c++14', install : true)