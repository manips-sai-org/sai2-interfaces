#include "controller/RobotControllerRedisInterface.h"

int main(int argc, char** argv) {
	std::string config_file = "resources/panda_controller_config.xml";
	Sai2Interfaces::RobotControllerRedisInterface controller(config_file);
	controller.run();
	return 0;
}