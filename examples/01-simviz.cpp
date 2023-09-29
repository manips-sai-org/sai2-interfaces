#include "simviz/SimVizRedisInterface.h"

int main(int argc, char** argv) {
    std::string world_file = "resources/world3.urdf";
    Sai2Interfaces::SimVizRedisInterface simviz(world_file);
    simviz.run();
    return 0;
}