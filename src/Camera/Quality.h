#ifndef QUALITY_H
#define QUALITY_H
#include <vector>
using namespace std;
class Quality
{
private:
    int width;
    int height;
    int framerate;
    static vector<Quality> quality_table;

public:
    enum QualityLevel {
        LOW,
        MEDIUM,
        HIGH
    } current_quality;
    Quality(int quality)
    {
        width = quality_table[quality].width;
        height = quality_table[quality].height;
        framerate = quality_table[quality].framerate;
    }
    Quality(int width_, int height_, int framerate_) : width(width_),
        height(height_),
        framerate(framerate_)
    {
    }
    // overload == operator
    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }

    int getFramerate() const
    {
        return framerate;
    }

    QualityLevel get_quality_level()
    {
        switch (height) {
        case 240:
            return LOW;
        case 480:
            return MEDIUM;
        case 720:
            return HIGH;
        default:
            return LOW;
        };
    }

    static Quality get_quality(QualityLevel quality_level)
    {
        switch (quality_level) {
        case LOW:
            return Quality(320, 240, 30);
        case MEDIUM:
            return Quality(640, 480, 30);
        case HIGH:
            return Quality(1280, 720, 30);
        default:
            return Quality(320, 240, 30);
        };
    }

    // Hacky lookup table! Sorry folks!
    static Quality int_to_Quality(int qual)
    {
        return quality_table[qual];
    }

    static int Quality_to_int(Quality q)
    {
        for (unsigned long int i = 0; i < quality_table.size(); i++) {
            if (quality_table[i] == q) {
                return i;
            }
        }
        return -1;
    }

    bool operator==(Quality &rhs) const
    {
        if (width == rhs.width && height == rhs.height && framerate == rhs.framerate) {
            return true;
        }
        return false;
    }
};

#endif