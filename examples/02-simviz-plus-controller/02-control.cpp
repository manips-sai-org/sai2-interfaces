#include "controller/RobotControllerConfigParser.h"
#include "controller/RobotControllerRedisInterface.h"

int main(int argc, char** argv) {
	std::string config_file = "resources/panda_controller_config.xml";
	Sai2Interfaces::RobotControllerConfigParser parser;
	Sai2Interfaces::RobotControllerRedisInterface controller(
		parser.parseConfig(config_file)[0]);
	controller.run();
	return 0;
}