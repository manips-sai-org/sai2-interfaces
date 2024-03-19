#include "MainRedisInterface.h"

#include <signal.h>
#include <tinyxml2.h>

#include "controller/RobotControllerConfigParser.h"
#include "simviz/SimVizConfigParser.h"

namespace {

const std::string CONFIG_FILE_NAME_KEY =
	"sai2::interfaces::main_interface::config_file_name";
const std::string RESET_KEY = "sai2::interfaces::main_interface::reset";

std::atomic<bool> controllers_stop_signal = false;
std::atomic<bool> simviz_stop_signal = false;
std::atomic<bool> external_stop_signal = false;
void stop(int i) { external_stop_signal = true; }

bool start_simviz = false;
bool simviz_was_stop_by_config_change = false;
bool sim_initialized = false;

}  // namespace

namespace Sai2Interfaces {

MainRedisInterface::MainRedisInterface(const std::string& config_folder_path,
									   const std::string& config_file_name)
	: _config_folder_path(config_folder_path),
	  _config_file_name(config_file_name) {
	// signal handling
	signal(SIGABRT, &stop);
	signal(SIGTERM, &stop);
	signal(SIGINT, &stop);

	// thread for running the interface loop
	std::thread interface_thread(&MainRedisInterface::runInterfaceLoop, this);

	// main loop that runs the graphics and simulation if needed
	while (!external_stop_signal) {
		if (start_simviz) {
			_simviz_interface =
				make_unique<SimVizRedisInterface>(*_simviz_config.get(), false);
			sim_initialized = true;
			_simviz_interface->run(simviz_stop_signal);
			if (simviz_was_stop_by_config_change) {
				simviz_was_stop_by_config_change = false;
				start_simviz = false;
			} else {
				external_stop_signal = true;
			}
		}
		_simviz_interface.reset();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	stopRunningControllers();
	interface_thread.join();
}

bool MainRedisInterface::parseConfig(const std::string& config_file_name) {
	const std::string config_file_path =
		_config_folder_path + "/" + config_file_name;

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(config_file_path.c_str()) != tinyxml2::XML_SUCCESS) {
		std::cout << "WARNING: Could not load config file: " << config_file_path
				  << std::endl;
		return false;
	}

	_config_file_name = config_file_name;

	_simviz_config = nullptr;
	if (doc.FirstChildElement("simvizConfiguration")) {
		SimVizConfigParser simviz_parser;
		_simviz_config = make_unique<SimVizConfig>(
			simviz_parser.parseConfig(config_file_path));
	}

	_controllers_configs.clear();
	if (doc.FirstChildElement("robotControlConfiguration")) {
		RobotControllerConfigParser controller_parser;
		_controllers_configs = controller_parser.parseConfig(config_file_path);
	}
	return true;
}

void MainRedisInterface::generateUiFile() {}

void MainRedisInterface::runInterfaceLoop() {
	Sai2Common::RedisClient redis_client;
	redis_client.connect();

	redis_client.set(CONFIG_FILE_NAME_KEY, _config_file_name);
	redis_client.setBool(RESET_KEY, true);

	while (!external_stop_signal) {
		std::string new_config_file_name =
			redis_client.get(CONFIG_FILE_NAME_KEY);

		if (_config_file_name != new_config_file_name ||
			redis_client.getBool(RESET_KEY)) {
			bool succes = parseConfig(new_config_file_name);
			if (succes) {
				generateUiFile();
				reset();
			}
			redis_client.setBool(RESET_KEY, false);
			redis_client.set(CONFIG_FILE_NAME_KEY, _config_file_name);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	controllers_stop_signal = true;
	simviz_stop_signal = true;
}

void MainRedisInterface::reset() {
	// first stop controllers
	stopRunningControllers();

	// next handle simulation
	if (_simviz_config && !_simviz_interface) {
		start_simviz = true;
		sim_initialized = false;
		while (!sim_initialized) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	} else if (!_simviz_config && _simviz_interface) {
		simviz_stop_signal = true;
		simviz_was_stop_by_config_change = true;
	} else if (_simviz_config && _simviz_interface) {
		_simviz_interface->reset(*_simviz_config);
		while (!_simviz_interface->isResetComplete()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	// finally start new controllers
	startNewControllers();
}

void MainRedisInterface::startNewControllers() {
	controllers_stop_signal = false;
	_controllers_threads.clear();
	for (const auto& config : _controllers_configs) {
		_controllers_interfaces[config.robot_name] =
			make_unique<RobotControllerRedisInterface>(config, false);
		_controllers_threads.push_back(std::thread([&]() {
			_controllers_interfaces.at(config.robot_name)
				->run(controllers_stop_signal);
		}));
	}
}

void MainRedisInterface::stopRunningControllers() {
	controllers_stop_signal = true;
	for (auto& thread : _controllers_threads) {
		thread.join();
	}
	_controllers_threads.clear();
	_controllers_interfaces.clear();
}

}  // namespace Sai2Interfaces