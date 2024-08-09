#include "simviz/SimVizRedisInterface.h"
#include "simviz/SimVizConfigParser.h"

int main(int argc, char** argv) {
	Sai2Model::URDF_FOLDERS["WORLD_FILES_FOLDER"] =
		std::string(EXAMPLE_FOLDER_PATH) + "/world_files";

	std::string config_file =
		std::string(EXAMPLE_FOLDER_PATH) + "/config_files/aa_detailled_panda_simviz_only.xml";

	Sai2Interfaces::SimVizConfigParser parser;
	Sai2Interfaces::SimVizConfig config = parser.parseConfig(config_file);

	Sai2Interfaces::SimVizRedisInterface simviz(config);
	simviz.run();

	return 0;
}