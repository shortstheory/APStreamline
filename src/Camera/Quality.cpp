#include "Quality.h"
#include <vector>
using namespace std;

vector<Quality> Quality::quality_table = {Quality(320, 240, 15), Quality(640, 480, 15), Quality(1280, 720, 15), Quality(320, 240, 30), Quality(640, 480, 30), Quality(1280, 720, 30), Quality(320, 240, 60), Quality(640, 480, 60), Quality(1280, 720, 60)};