#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <gst/gst.h>

#ifdef __amd64__
static const guint32 MAX_STEADY_BITRATE = 6000;
#endif
#ifdef __arm__
static const guint32 MAX_STEADY_BITRATE = 2500;
#endif

static const guint32 MIN_STEADY_BITRATE = 500;
static const guint32 INC_STEADY_BITRATE = 750;
static const guint32 DEC_STEADY_BITRATE = 1000;

static const guint32 MAX_CONGESTION_BITRATE = 1000;
static const guint32 MIN_CONGESTION_BITRATE = 100;
static const guint32 INC_CONGESTION_BITRATE = 125;
static const guint32 DEC_CONGESTION_BITRATE = 500;

static const guint32 LOW_QUAL_BITRATE = 500;
static const guint32 MED_QUAL_BITRATE = 1000;
static const guint32 HIGH_QUAL_BITRATE = 3000;

static const guint32 I_FRAME_INTERVAL = 10;
static const guint32 SUCCESSFUL_TRANSMISSION = 5;

#endif