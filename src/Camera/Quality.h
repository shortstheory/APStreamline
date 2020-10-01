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
    enum class QualityLevel {
        LOW,
        MEDIUM,
        HIGH
    } current_quality;

    Quality(int quality);
    Quality(int width_, int height_, int framerate_);
    // overload == operator
    int getWidth() const;
    int getHeight() const;
    int getFramerate() const;
    QualityLevel get_quality_level();
    static Quality get_quality(QualityLevel quality_level);
    static Quality int_to_Quality(int qual);
    static int Quality_to_int(Quality q);
    bool operator==(Quality &rhs) const;
};

#endif