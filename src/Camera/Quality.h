#ifndef QUALITY_H
#define QUALITY_H

class Quality
{
public:
    enum class Level {
        LOW,
        MEDIUM,
        HIGH
    };

    Quality(Level _resolution, Level _framerate);
    // overload == operator
    static Quality int_to_Quality(int qual);
    int to_int() const;
    Level getResolution() const;
    Level getFramerate() const;
private:
    Level resolution;
    Level framerate;
};

#endif