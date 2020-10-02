#include "MJPGCamera.h"
#include <gst/gst.h>
#include <cstdlib>

MJPGCamera::MJPGCamera(string device, Quality q) : Camera(device, q), encoder(nullptr), capsfilter(nullptr)
{
    Config camera_config;
    Config quality_config;

    // Read the file. If there is an error, report it and exit.
    try {
        cout << "Reading config" << endl;
        camera_config.readFile("config/MJPGCamera.cfg");
        quality_config.readFile("config/settings.cfg");
        read_configuration(camera_config.getRoot(), quality_config.getRoot());
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading file." << std::endl;
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
    }
}

bool MJPGCamera::set_element_references(GstElement *pipeline)
{
    encoder = gst_bin_get_by_name(GST_BIN(pipeline), encoder_name.c_str());
    capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    if (encoder && capsfilter) {
        return true;
    }
    return false;
}

bool MJPGCamera::set_bitrate(guint32 _bitrate)
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

bool MJPGCamera::set_quality(Quality q)
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

bool MJPGCamera::read_configuration(Setting &camera_config, Setting &quality_config)
{
    encoder_name = static_cast<const char *>(camera_config.lookup("camera.properties.encoder_name"));
    return Camera::read_configuration(camera_config, quality_config);
}

string MJPGCamera::generate_launch_string() const
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
    regex enc("%encoder");
    regex br("%bitrate");
    string result;
    result = regex_replace(launch_string, d, device_path);
    result = regex_replace(result, cf, capsfilter_string);
    result = regex_replace(result, enc, encoder_name);
    result = regex_replace(result, br, to_string(launch_bitrate));
    return result;
}

void MJPGCamera::improve_quality(bool congested)
{
    set_bitrates_constants(congested);
    set_bitrate(bitrate + increment_bitrate);
    if (current_quality.getResolution() == Quality::Level::LOW && bitrate > medium_bitrate) {
        Quality mediumQuality(Quality::Level::MEDIUM, Quality::Level::MEDIUM);
        set_quality(mediumQuality);
    } else if (current_quality.getResolution() == Quality::Level::MEDIUM && bitrate > high_bitrate) {
        Quality highQuality(Quality::Level::HIGH, Quality::Level::MEDIUM);
        set_quality(highQuality);
    }
}

void MJPGCamera::degrade_quality(bool congested)
{
    set_bitrates_constants(congested);
    set_bitrate(bitrate - decrement_bitrate);
    if (current_quality.getResolution() == Quality::Level::MEDIUM && bitrate < medium_bitrate) {
        Quality lowQuality(Quality::Level::LOW, Quality::Level::MEDIUM);
        set_quality(lowQuality);
    } else if (current_quality.getResolution() == Quality::Level::HIGH && bitrate < high_bitrate) {
        Quality mediumQuality(Quality::Level::MEDIUM, Quality::Level::MEDIUM);
        set_quality(mediumQuality);
    }
}