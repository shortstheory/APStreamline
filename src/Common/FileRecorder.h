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
#include <atomic>

using namespace std;

class FileRecorder
{
private:
    bool recording;
public:
    GstElement* file_recorder_bin;
    GstElement* file_sink;
    GstElement* file_queue;
    GstElement* file_h264_parser;
    GstElement* mux;
    GstElement* pipeline;
    GstElement* tee;
    GstPad* tee_file_pad;
    GstPad* queue_pad;

    bool stop_recording;

    FileRecorder();
    bool init_file_recorder(GstElement* _pipeline, GstElement* _tee);
    bool disable_recorder();
    bool get_recording();
};

#endif