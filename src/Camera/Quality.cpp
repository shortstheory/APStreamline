#include "Quality.h"

Quality::Quality(Level _resolution, Level _framerate) : resolution(_resolution), framerate(_framerate)
{
}

// overload == operator
// Hacky lookup table! Sorry folks!
// FIXME: Fill in this method
Quality Quality::int_to_Quality(int quality)
{
    int framerate;
    int resolution;
    framerate = quality / 3;
    resolution = quality % 3;

    Quality::Level resolution_level;
    Quality::Level framerate_level;

    if (framerate == 0) {
        framerate_level = Quality::Level::LOW;
    } else if (framerate == 1) {
        framerate_level = Quality::Level::MEDIUM;
    } else if (framerate == 2) {
        framerate_level = Quality::Level::HIGH;
    } else {
        framerate_level = Quality::Level::MEDIUM;
    }

    if (resolution == 0) {
        resolution_level = Quality::Level::LOW;
    } else if (resolution == 1) {
        resolution_level = Quality::Level::MEDIUM;
    } else if (resolution == 2) {
        resolution_level = Quality::Level::HIGH;
    } else {
        resolution_level = Quality::Level::MEDIUM;
    }
    return Quality(resolution_level, framerate_level);
}

int Quality::to_int() const
{
    int result;
    if (framerate == Level::LOW) {
        result = 0;
    } else if (framerate == Level::MEDIUM) {
        result = 3;
    } else if (framerate == Level::HIGH) {
        result = 6;
    }
    if (resolution == Level::LOW) {
        result += 0;
    } else if (resolution == Level::MEDIUM) {
        result += 1;
    } else if (resolution == Level::HIGH) {
        result += 2;
    }
    return result;
}

Quality::Level Quality::getResolution() const
{
    return resolution;
}

Quality::Level Quality::getFramerate() const
{
    return framerate;
}