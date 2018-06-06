#ifndef QOS_REPORT_H
#define QOS_REPORT_H

#include <gst/gst.h>
class QoSReport {
private:
    guint8 fraction_lost;
    gfloat estimated_bitrate;
    gfloat encoding_bitrate;
    gfloat rtt;
public:
    QoSReport()
    {
    }

    QoSReport(guint8 _fl, gfloat _estd_br, gfloat _enc_br, gfloat _rtt) :
    fraction_lost(_fl), estimated_bitrate(_estd_br), encoding_bitrate(_enc_br),
    rtt(_rtt)
    {
    }
};

#endif