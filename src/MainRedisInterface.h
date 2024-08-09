#ifndef SAI2_INTERFACES_MAIN_REDIS_INTERFACE_H
#define SAI2_INTERFACES_MAIN_REDIS_INTERFACE_H

#include <string>

#include "controller/RobotControllerRedisInterface.h"
#include "simviz/SimVizRedisInterface.h"

namespace Sai2Interfaces {

/**
 * @brief Main class to run for launching a simulation and controllers from the
 * custom xml config files and provide an easy method of interaction via redis
 *
 */
class MainRedisInterface {
public:
	/**
	 * @brief The constructor for the MainRedisInterface class. It does all the
	 * work: read the config file, generate the ui file (it is generated in the
	 * config_folder_path/webui_generated_file/ folder), start the simviz
	 * interface and the controllers interfaces.
	 *
	 * @param config_folder_path The path to the config files. This is required
	 * in order to switch between configs. Only the config files in this folder
	 * can be used by this application.
	 * @param config_file_name The name of the initial config file. If not
	 * given, a config file can be sent via redis later.
	 */
	MainRedisInterface(const std::string& config_folder_path,
					   const std::string& config_file_name = "");
	~MainRedisInterface() = default;

	// disallow default, copy and move constructors
	MainRedisInterface(MainRedisInterface const&) = delete;
	MainRedisInterface(MainRedisInterface&&) = delete;
	MainRedisInterface& operator=(MainRedisInterface const&) = delete;
	MainRedisInterface& operator=(MainRedisInterface&&) = delete;
	MainRedisInterface() = delete;

private:
	void runInterfaceLoop();

	bool parseConfig(const std::string& config_file_name);
	void generateUiFile();

	void reset();
	void startNewControllers();
	void stopRunningControllers();

	std::unique_ptr<SimVizRedisInterface> _simviz_interface;
	std::map<std::string, std::unique_ptr<RobotControllerRedisInterface>>
		_controllers_interfaces;

	std::vector<std::thread> _controllers_threads;

	std::unique_ptr<SimVizConfig> _simviz_config;
	std::vector<RobotControllerConfig> _controllers_configs;

	std::string _config_folder_path;
	std::string _config_file_name;
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_MAIN_REDIS_INTERFACE_H