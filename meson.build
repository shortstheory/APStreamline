project('adaptive-streaming', 'cpp', default_options: [
    'cpp_std=c++14'
  ])
gst_dep = dependency('gstreamer-1.0')
gst_rtp_dep = dependency('gstreamer-rtp-1.0')
src = ['src/main.cpp', 'src/AdaptiveStreaming.cpp', 'src/QoSEstimator.cpp']
executable('adaptive_streaming', sources: src, dependencies: [gst_dep, gst_rtp_dep], cpp_args : '-std=c++14')
