#include "Quality.h"

Quality::Quality(Level _resolution, Level _framerate) : resolution(_resolution), framerate(_framerate)
{
}

// overload == operator
// Hacky lookup table! Sorry folks!
// FIXME: Fill in this method
Quality Quality::int_to_Quality(int qual)
{
    return Quality(Level::MEDIUM, Level::MEDIUM);
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