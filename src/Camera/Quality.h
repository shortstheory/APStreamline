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
};

#endif