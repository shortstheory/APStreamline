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
public:
    GstElement* file_recorder_bin;
    GstElement* file_sink;
    GstElement* file_queue;
    GstElement* file_h264_parser;
    GstElement* mux;
    GstElement* pipeline;
    void init_file_recorder(GstElement* _pipeline)
    {
        pipeline = _pipeline;
        // file_recorder_bin = gst_bin_new(NULL);
        file_queue = gst_element_factory_make("queue", NULL);
        file_h264_parser = gst_element_factory_make("h264parse", NULL);
        mux = gst_element_factory_make("matroskamux", NULL);
        file_sink = gst_element_factory_make("filesink", NULL);

        string file_path;
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream ss;
        ss << put_time(&tm, "%d-%m-%Y_%H:%M:%S") << endl;
        file_path =  ss.str();
        file_path = "Video_" + file_path + ".mkv";

        g_object_set(G_OBJECT(file_sink), "location", file_path.c_str(), NULL);

        gst_bin_add_many(GST_BIN(pipeline), file_queue, file_h264_parser, mux, file_sink, NULL);
        gst_element_link_many(file_queue, file_h264_parser, mux, file_sink, NULL);
        gst_element_sync_state_with_parent(file_queue);
        gst_element_sync_state_with_parent(file_h264_parser);
        gst_element_sync_state_with_parent(mux);
        gst_element_sync_state_with_parent(file_sink);
    }

    void disable_recorder()
    {
        gst_bin_remove(GST_BIN(pipeline), file_queue);
        gst_bin_remove(GST_BIN(pipeline), file_h264_parser);
        gst_bin_remove(GST_BIN(pipeline), mux);
        gst_bin_remove(GST_BIN(pipeline), file_sink);

        gst_element_set_state(file_queue, GST_STATE_NULL);
        gst_element_set_state(file_h264_parser, GST_STATE_NULL);
        gst_element_set_state(mux, GST_STATE_NULL);
        gst_element_set_state(file_sink, GST_STATE_NULL);

        gst_object_unref(file_h264_parser);
        gst_object_unref(file_queue);
        gst_object_unref(mux);
        gst_object_unref(file_sink);
    }

};

#endif