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
public:
    void init_file_recorder()
    {
        // this gets linked to tee, but doesn't get deinitialised
        file_queue = gst_element_factory_make("queue", NULL);

        file_h264_parser = gst_element_factory_make("h264parse", NULL);
        mux = gst_element_factory_make("matroskamux", NULL);
        file_sink = gst_element_factory_make("filesink", NULL);
        file_recorder_bin = gst_bin_new(NULL);

        string file_path;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream ss;
        ss << put_time(&tm, "%d-%m-%Y_%H:%M:%S") << endl;
        file_path =  ss.str();
        file_path = "Video_" + file_path + ".mkv";

        g_object_set(G_OBJECT(file_sink), "location", file_path.c_str(), NULL);

        gst_bin_add_many(GST_BIN(file_recorder_bin), file_h264_parser, mux, file_sink, NULL);
        // // see if we need ghost pads or if we can get away without it here
        gst_element_link_many(file_queue, file_h264_parser, mux, file_sink, NULL);
    }
};

#endif