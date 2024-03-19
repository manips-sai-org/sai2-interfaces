#include <thread>

#include "simviz/SimVizConfigParser.h"
#include "simviz/SimVizRedisInterface.h"

const std::string config_file_1 = "resources/simviz_config_panda.xml";
const std::string config_file_2 = "resources/simviz_config_kukas.xml";
const std::string config_file_3 = "resources/simviz_config_pendulum.xml";

void auxThreadRun(std::atomic<bool>& stop_simviz,
				  Sai2Interfaces::SimVizRedisInterface& simviz) {
	Sai2Interfaces::SimVizConfigParser parser;

	for (int i = 0; i < 100; ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (stop_simviz) {
			return;
		}
	}
	simviz.reset(parser.parseConfig(config_file_2));

	for (int i = 0; i < 100; ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (stop_simviz) {
			return;
		}
	}
	simviz.reset(parser.parseConfig(config_file_3));

	for (int i = 0; i < 100; ++i) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (stop_simviz) {
			return;
		}
	}
	stop_simviz = true;
}

int main(int argc, char** argv) {
	Sai2Interfaces::SimVizConfigParser parser;
	Sai2Interfaces::SimVizRedisInterface simviz(
		parser.parseConfig(config_file_1));

	std::atomic<bool> stop_simviz = false;
	std::thread aux_thread(auxThreadRun, std::ref(stop_simviz),
						   std::ref(simviz));

	simviz.run(stop_simviz);
	stop_simviz = true;

	aux_thread.join();

	return 0;
}