#include "AR0521Camera.h"
#include <gst/gst.h>
#include <cstdlib>

AR0521Camera::AR0521Camera(string device, Quality q) : Camera(device, q), encoder(nullptr), capsfilter_element(nullptr)
{
    Config camera_config;
    Config quality_config;

    // Read the file. If there is an error, report it and exit.
    try {
        cout << "Reading AR0521 config" << endl;
        camera_config.readFile("config/AR0521Camera.cfg");
        quality_config.readFile("config/settings.cfg");
        read_configuration(camera_config.getRoot(), quality_config.getRoot());
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading file." << std::endl;
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
    }
}

bool AR0521Camera::set_element_references(GstElement *pipeline)
{
    encoder = gst_bin_get_by_name(GST_BIN(pipeline), "encoder");
    capsfilter_element = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    if (encoder && capsfilter_element) {
        return true;
    }
    return false;
}

bool AR0521Camera::set_bitrate(guint32 _bitrate)
{
    if (_bitrate < min_bitrate) {
        bitrate = min_bitrate;
    } else if (_bitrate > max_bitrate) {
        bitrate = max_bitrate;
    } else {
        bitrate = _bitrate;
    }
    // some drivers using bps instead of kbps
    g_object_set(G_OBJECT(encoder), "bitrate", 1000*bitrate, NULL);
    return true;
}

bool AR0521Camera::set_quality(Quality q)
{
    // Changing quality isn't supported, but one can play with the bitrate
    return false;
}

string AR0521Camera::generate_launch_string() const
{
    string capsfilter_string;
    guint32 launch_bitrate;
    switch (current_quality.getResolution()) {
    case Quality::Level::LOW:
        launch_bitrate = low_bitrate;
        break;
    case Quality::Level::MEDIUM:
        launch_bitrate = medium_bitrate;
        break;
    case Quality::Level::HIGH:
        launch_bitrate = high_bitrate;
        break;
    };
    launch_bitrate *= 1000;
    capsfilter_string = generate_capsfilter();
    regex d("%device");
    regex cf("%capsfilter");
    regex br("%bitrate");
    string result;
    result = regex_replace(launch_string, d, device_path);
    result = regex_replace(result, cf, capsfilter_string);
    result = regex_replace(result, br, to_string(launch_bitrate));
    return result;
}

string AR0521Camera::generate_capsfilter() const
{
    regex w("%width");
    regex h("%height");

    string caps;
    caps = capsfilter;

    pair<int, int> res;
    res = resolutions.at(current_quality.getResolution());
    // changing framerate not supported on AR0521
    caps = regex_replace(caps, w, to_string(res.first));
    caps = regex_replace(caps, h, to_string(res.second));
    return caps;
}

