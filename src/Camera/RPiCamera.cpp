#include "RPiCamera.h"
#include <gst/gst.h>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>

RPiCamera::RPiCamera(string device, Quality q) : Camera(device, q), src(nullptr), capsfilter(nullptr)
{   
    Config camera_config;
    Config quality_config;

    // Read the file. If there is an error, report it and exit.
    try {
        cout << "Reading config" << endl;
        camera_config.readFile("config/RPiCamera.cfg");
        quality_config.readFile("config/settings.cfg");
        read_configuration(camera_config.getRoot(), quality_config.getRoot());
    } catch (const FileIOException &fioex) {
        cerr << "I/O error while reading file." << std::endl;
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
    }
}

bool RPiCamera::set_element_references(GstElement *pipeline)
{
    src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    capsfilter = gst_bin_get_by_name(GST_BIN(pipeline), "capsfilter");
    if (src && capsfilter) {
        return true;
    }
    return false;
}

bool RPiCamera::set_bitrate(guint32 _bitrate)
{
    if (_bitrate < min_bitrate) {
        bitrate = min_bitrate;
    } else if (_bitrate > max_bitrate) {
        bitrate = max_bitrate;
    } else {
        bitrate = _bitrate;
    }

    int v4l2_cam_fd;
    g_object_get(src, "device-fd", &v4l2_cam_fd, NULL);
    if (v4l2_cam_fd > 0) {
        v4l2_control bitrate_ctrl;
        bitrate_ctrl.id = V4L2_CID_MPEG_VIDEO_BITRATE;
        bitrate_ctrl.value = bitrate*1000;
        if (ioctl(v4l2_cam_fd, VIDIOC_S_CTRL, &bitrate_ctrl) == -1) {
            return false;
        }
    }
    return true;
}

bool RPiCamera::set_quality(Quality q)
{
    // Changing quality isn't supported, but one can play with the bitrate
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

bool RPiCamera::read_configuration(Setting &camera_config, Setting &quality_config)
{
    return Camera::read_configuration(camera_config, quality_config);
}

string RPiCamera::generate_launch_string() const
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
