#include "ZEDCamera.h"
#include <gst/gst.h>
#include <cstdlib>

ZEDCamera::ZEDCamera(string device, Quality q) : Camera(device, q), encoder(nullptr), capsfilter(nullptr)
{
    Config camera_config;
    Config quality_config;

    // Read the file. If there is an error, report it and exit.
    try {
        cout << "Reading ZED config" << endl;
        camera_config.readFile("config/ZEDCamera.cfg");
        quality_config.readFile("config/settings.cfg");
        read_configuration(camera_config.getRoot(), quality_config.getRoot());
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading file." << std::endl;
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
    }
}

bool ZEDCamera::set_element_references(GstElement *pipeline)
{
    encoder = gst_bin_get_by_name(GST_BIN(pipeline), "encoder");
    capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    if (encoder && capsfilter) {
        return true;
    }
    return false;
}

bool ZEDCamera::set_bitrate(guint32 _bitrate)
{
    if (_bitrate < min_bitrate) {
        bitrate = min_bitrate;
    } else if (_bitrate > max_bitrate) {
        bitrate = max_bitrate;
    } else {
        bitrate = _bitrate;
    }
    g_object_set(G_OBJECT(encoder), "bitrate", bitrate, NULL);
    return true;
}

bool ZEDCamera::set_quality(Quality q)
{
    string capsfilter_string;
    // TODO: add checks for if Q is valid or not
    current_quality = q;
    capsfilter_string = generate_capsfilter();
    GstCaps *caps;
    caps = gst_caps_from_string(capsfilter_string.c_str());
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);
    return true;
}

string ZEDCamera::generate_launch_string() const
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
