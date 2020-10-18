struct QoSReport {
    unsigned char fraction_lost;
    float estimated_bitrate;
    float encoding_bitrate;
    float rtt;
    float buffer_occ;

    QoSReport()
    {
    }

    QoSReport(unsigned char _fl, float _estd_br, float _enc_br, float _rtt, float bo) :
        fraction_lost(_fl), estimated_bitrate(_estd_br), encoding_bitrate(_enc_br),
        rtt(_rtt), buffer_occ(bo)
    {
    }
};
