#ifndef QUALITY_H
#define QUALITY_H

using namespace std;
class Quality
{
private:
    int width;
    int height;
    int framerate;

public:
    enum QualityLevel
    {
        LOW,
        MEDIUM,
        HIGH
    } current_quality;
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
        switch (height)
        {
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
        switch (quality_level)
        {
        case LOW:
            return Quality(320, 240, 30);
        case MEDIUM:
            return Quality(640, 480, 30);
        case HIGH:
            return Quality(1280, 720, 30);
        };
    }

    // Hacky lookup table! Sorry folks!
    static Quality int_to_Quality(int qual)
    {
        Quality quality(0,0,0);
        if (qual == 0) {
            quality = Quality(320,240,15);
        } else if (qual == 1) {
            quality = Quality(640,480,15);
        } else if (qual == 2) {
            quality = Quality(1280,720,15);
        } else if (qual == 3) {
            quality = Quality(320,240,30);
        } else if (qual == 4) {
            quality = Quality(640,480,30);
        } else if (qual == 5) {
            quality = Quality(1280,720,30);
        } else if (qual == 6) {
            quality = Quality(320,240,60);
        } else if (qual == 7) {
            quality = Quality(640,480,60);
        } else if (qual == 8) {
            quality = Quality(1280,720,60);
        }
    }

    bool operator==(Quality& rhs) const
    {
        if (width == rhs.width && height == rhs.height && framerate == rhs.framerate) {
            return true;
        }
        return false;
    }
};

#endif