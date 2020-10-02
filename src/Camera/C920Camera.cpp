#include "C920Camera.h"
#include <gst/gst.h>
#include <cstdlib>

C920Camera::C920Camera(string device, Quality q) : Camera(device, q), device(nullptr), capsfilter(nullptr)
{
    Config camera_config;
    Config quality_config;

    // Read the file. If there is an error, report it and exit.
    try {
        cout << "Reading config" << endl;
        camera_config.readFile("config/C920Camera.cfg");
        quality_config.readFile("config/settings.cfg");
        read_configuration(camera_config.getRoot(), quality_config.getRoot());
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading file." << std::endl;
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
    }
}

bool C920Camera::set_element_references(GstElement *pipeline)
{
    device = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    if (device && capsfilter) {
        return true;
    }
    return false;
}

bool C920Camera::set_bitrate(guint32 _bitrate)
{
    if (_bitrate < min_bitrate) {
        bitrate = min_bitrate;
    } else if (_bitrate > max_bitrate) {
        bitrate = max_bitrate;
    } else {
        bitrate = _bitrate;
    }
    // some drivers using bps instead of kbps
    g_object_set(G_OBJECT(device), "average-bitrate", 1000*bitrate, NULL);
    return true;
}

bool C920Camera::set_quality(Quality q)
{
    // We can't change the resolution in H264 mode, though framerate might work
    if (q.getResolution() == current_quality.getResolution()) {
        string capsfilter_string;
        current_quality = q;
        capsfilter_string = generate_capsfilter();
        GstCaps *caps;
        caps = gst_caps_from_string(capsfilter_string.c_str());
        g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
        gst_caps_unref(caps);
        return true;
    }
    return false;
}

bool C920Camera::read_configuration(Setting &camera_config, Setting &quality_config)
{
    return Camera::read_configuration(camera_config, quality_config);
}

string C920Camera::generate_launch_string() const
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

// Sadly the stream crashes if we change resolution in H264 mode, so we only have
// bitrate control
void C920Camera::improve_quality(bool congested)
{
    set_bitrates_constants(congested);
    set_bitrate(bitrate + increment_bitrate);
}

void C920Camera::degrade_quality(bool congested)
{
    set_bitrates_constants(congested);
    set_bitrate(bitrate - decrement_bitrate);
}