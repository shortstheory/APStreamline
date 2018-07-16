#ifndef FILE_RECORDER_H
#define FILE_RECORDER_H

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <gst/rtp/gstrtcpbuffer.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <ctime>

class FileRecorder {
    GstElement* file_recorder_bin;
    GstElement* file_sink;
    GstElement* file_queue;
    GstElement* file_h264_parser;
    GstElement* mux;

};

#endif