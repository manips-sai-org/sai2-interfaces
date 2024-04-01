#include <thread>

#include "simviz/SimVizConfigParser.h"
#include "simviz/SimVizRedisInterface.h"

const std::string config_folder =
	std::string(EXAMPLE_FOLDER_PATH) + "/01-simviz-only";
const std::string config_file_key =
	"sai2::interfaces::simviz::config_file_name";
const std::string reset_key =
	"sai2::interfaces::simviz::reset";

bool stop_aux_thread = false;

void auxThreadRun(Sai2Interfaces::SimVizRedisInterface& simviz) {
	Sai2Common::RedisClient redis_client;
	redis_client.connect();

	Sai2Interfaces::SimVizConfigParser parser;
	std::string config_file_name = "config_panda.xml";
	bool reset = false;
	redis_client.set(config_file_key, config_file_name);
	redis_client.setBool(reset_key, reset);

	while (!stop_aux_thread) {
		std::string new_config_file = redis_client.get(config_file_key);
		reset = redis_client.getBool(reset_key);
		if (reset) {
			config_file_name = new_config_file;
			simviz.reset(
				parser.parseConfig(config_folder + "/" + config_file_name));
			redis_client.setBool(reset_key, false);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int main(int argc, char** argv) {
	// add world file folder to search path for parser
	Sai2Model::URDF_FOLDERS["WORLD_FILES_FOLDER"] =
		std::string(EXAMPLE_FOLDER_PATH) + "/world_files";

	Sai2Interfaces::SimVizConfigParser parser;
	Sai2Interfaces::SimVizRedisInterface simviz(
		parser.parseConfig(config_folder + "/config_panda.xml"));

	std::thread aux_thread(auxThreadRun, std::ref(simviz));

	simviz.run();
	stop_aux_thread = true;
	aux_thread.join();

	return 0;
}