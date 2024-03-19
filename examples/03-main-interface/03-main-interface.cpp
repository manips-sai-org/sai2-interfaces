#include "MainRedisInterface.h"

int main(int argc, char** argv) {
	std::string config_folder_path = "resources";
	std::string config_file = "main_config.xml";

	Sai2Interfaces::MainRedisInterface main_interface(config_folder_path);

	return 0;
}