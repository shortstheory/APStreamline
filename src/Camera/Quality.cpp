#include "Quality.h"
#include <vector>
using namespace std;

vector<Quality> Quality::quality_table = {
        Quality(320, 240, 15), Quality(640, 480, 15), Quality(1280, 720, 15), 
        Quality(320, 240, 30), Quality(640, 480, 30), Quality(1280, 720, 30), 
        Quality(320, 240, 60), Quality(640, 480, 60), Quality(1280, 720, 60)
};

Quality::Quality(int quality)
{
    width = quality_table[quality].width;
    height = quality_table[quality].height;
    framerate = quality_table[quality].framerate;
}

Quality::Quality(int width_, int height_, int framerate_) : width(width_),
    height(height_),
    framerate(framerate_)
{
}

// overload == operator
int Quality::getWidth() const
{
    return width;
}

int Quality::getHeight() const
{
    return height;
}

int Quality::getFramerate() const
{
    return framerate;
}

Quality::QualityLevel Quality::get_quality_level()
{
    switch (height) {
    case 240:
        return QualityLevel::LOW;
    case 480:
        return QualityLevel::MEDIUM;
    case 720:
        return QualityLevel::HIGH;
    default:
        return QualityLevel::LOW;
    };
}

Quality Quality::get_quality(Quality::QualityLevel quality_level)
{
    switch (quality_level) {
    case Quality::QualityLevel::LOW:
        return Quality(320, 240, 30);
    case Quality::QualityLevel::MEDIUM:
        return Quality(640, 480, 30);
    case Quality::QualityLevel::HIGH:
        return Quality(1280, 720, 30);
    default:
        return Quality(320, 240, 30);
    };
}

// Hacky lookup table! Sorry folks!
Quality Quality::int_to_Quality(int qual)
{
    return quality_table[qual];
}

int Quality::Quality_to_int(Quality q)
{
    for (unsigned long int i = 0; i < quality_table.size(); i++) {
        if (quality_table[i] == q) {
            return i;
        }
    }
    return -1;
}

bool Quality::operator==(Quality &rhs) const
{
    if (width == rhs.width && height == rhs.height && framerate == rhs.framerate) {
        return true;
    }
    return false;
}