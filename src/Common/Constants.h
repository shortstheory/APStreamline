#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <gst/gst.h>
#include <vector>

using namespace std;

static const guint32 IPC_BUFFER_SIZE = 10000;

static const guint32 SUCCESSFUL_TRANSMISSION = 5;

static const int AUTO_PRESET = 1024;
static const string SOCKET_PATH = "/tmp/rtsp_server.sock";


#endif