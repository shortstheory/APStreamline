#include "MJPGCamera.h"
#include <gst/gst.h>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>

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
    get_supported_qualities();
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

bool MJPGCamera::get_supported_qualities()
{
    int fd;
    fd = open(device_path.c_str(), O_RDONLY);
    if (fd == -1) {
        return false;
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmtdesc fmt;
    v4l2_frmsizeenum frmsize;
    v4l2_frmivalenum frmival;

    memset(&fmt, 0, sizeof(fmt));
    memset(&frmsize, 0, sizeof(frmsize));
    memset(&frmival, 0, sizeof(frmival));
    fmt.type = type;
    fmt.index = 0;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) >= 0) {
        string description(reinterpret_cast<char*>(fmt.description));
        if (description.find("Motion-JPEG") != string::npos
        || description.find("MJPG") != string::npos
        || description.find("M.JPG") != string::npos) {
            break;
        }
        fmt.index++;
    }
    frmsize.pixel_format = fmt.pixelformat;


    supported_qualities = 0;

    for (frmsize.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0; frmsize.index++) {
        // go through all the possible resolution, but we're only looking for 240p, 480p, 720p
        if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
            Quality::Level resolutionLevel;
            if (frmsize.discrete.width == 320 && frmsize.discrete.height == 240) {
                resolutionLevel = Quality::Level::LOW;
            } else if (frmsize.discrete.width == 640 && frmsize.discrete.height == 480) {
                resolutionLevel = Quality::Level::MEDIUM;
            } else if (frmsize.discrete.width == 1280 && frmsize.discrete.height == 720) {
                resolutionLevel = Quality::Level::HIGH;
            } else {
                continue;
            }

            frmival.pixel_format = fmt.pixelformat;
            frmival.width = frmsize.discrete.width;
            frmival.height = frmsize.discrete.height;

            for (frmival.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0; frmival.index++) {
                int framerate = frmival.discrete.denominator;
                // For simplicity in maintenance, we only support 240p, 480p
                // and 720p. If a camera supports a resolution we use a bitmask
                // for saving its capabilities as it greatly simplifies our IPC
                Quality::Level framerateLevel;
                if (framerate == 15) {
                    framerateLevel = Quality::Level::LOW;
                } else if (framerate == 30) {
                    framerateLevel = Quality::Level::MEDIUM;
                } else if (framerate == 60) {
                    framerateLevel = Quality::Level::HIGH;
                }
                Quality q(resolutionLevel, framerateLevel);
                // populate the bitmask
                supported_qualities |= (1 << q.to_int());
            }
        }
    }
    close(fd);
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