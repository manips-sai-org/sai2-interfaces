#include "controller/RobotControllerConfigParser.h"
#include "controller/RobotControllerRedisInterface.h"

int main(int argc, char** argv) {
	// add world file folder to search path for parser
	Sai2Model::URDF_FOLDERS["WORLD_FILES_FOLDER"] =
		std::string(EXAMPLE_FOLDER_PATH) + "/world_files";

	// config file for simviz plus controller
	std::string config_file =
		std::string(EXAMPLE_FOLDER_PATH) +
		"/02-simviz-plus-controller/config_panda_simviz_controller.xml";
	Sai2Interfaces::RobotControllerConfigParser parser;
	Sai2Interfaces::RobotControllerRedisInterface controller(
		parser.parseConfig(config_file)[0]);
	controller.run();
	return 0;
}