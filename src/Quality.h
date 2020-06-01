#include <tuple>
using namespace std;
class Quality
{
private:
    int width;
    int height;
    int framerate;
public:
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
};