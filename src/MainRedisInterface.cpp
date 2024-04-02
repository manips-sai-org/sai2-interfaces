#include "MainRedisInterface.h"

#include <signal.h>
#include <tinyxml2.h>

#include <filesystem>

#include "controller/RobotControllerConfigParser.h"
#include "simviz/SimVizConfigParser.h"

namespace {

const std::string CONFIG_FILE_NAME_KEY =
	"sai2::interfaces::main_interface::config_file_name";
const std::string RESET_KEY = "sai2::interfaces::main_interface::reset";

const std::string WEBUI_TEMPLATE_FILE_PATH =
	std::string(UI_FOLDER) + "/web/html/webui_template.html";

std::atomic<bool> controllers_stop_signal = false;
std::atomic<bool> simviz_stop_signal = false;
std::atomic<bool> external_stop_signal = false;
void stop(int i) { ::external_stop_signal = true; }

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
	while (!::external_stop_signal) {
		if (start_simviz) {
			_simviz_interface =
				make_unique<SimVizRedisInterface>(*_simviz_config.get(), false);
			sim_initialized = true;
			_simviz_interface->run(::simviz_stop_signal);
			if (simviz_was_stop_by_config_change) {
				simviz_was_stop_by_config_change = false;
				start_simviz = false;
			} else {
				::external_stop_signal = true;
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

std::vector<std::string> getControllerNameAndTasksFromSingleConfig(
	const std::pair<
		std::string,
		std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>>>
		single_controller_config) {
	std::string controller_name = single_controller_config.first;
	std::string controller_tasks_names = "[";
	std::string controller_tasks_types = "[";
	for (int i = 0; i < single_controller_config.second.size(); i++) {
		const auto& task = single_controller_config.second.at(i);
		if (std::holds_alternative<JointTaskConfig>(task)) {
			controller_tasks_types += "\"joint_task\"";
			controller_tasks_names +=
				"\"" + get<JointTaskConfig>(task).task_name + "\"";
		} else if (std::holds_alternative<MotionForceTaskConfig>(task)) {
			controller_tasks_types += "\"motion_force_task\"";
			controller_tasks_names +=
				"\"" + get<MotionForceTaskConfig>(task).task_name + "\"";
		} else {
			std::cerr << "Error: Unknown task type in generating ui html.\n";
		}
		if (i != single_controller_config.second.size() - 1) {
			controller_tasks_names += ", ";
			controller_tasks_types += ", ";
		}
	}
	controller_tasks_names += "]";
	controller_tasks_types += "]";
	return std::vector<std::string>{controller_name, controller_tasks_names,
									controller_tasks_types};
}

std::vector<std::string> generateControllerNamesAndTasksForUI(
	const RobotControllerConfig& config) {
	std::string controller_names = "[";
	std::string controller_tasks_names = "[";
	std::string controller_tasks_types = "[";

	for (auto it = config.controllers_configs.begin();
		 it != config.controllers_configs.end(); ++it) {
		const auto& pair = *it;
		const auto& controller_name = pair.first;

		auto controller_ui_specs =
			getControllerNameAndTasksFromSingleConfig(pair);
		if (it != config.controllers_configs.begin()) {
			controller_names += ", ";
			controller_tasks_names += ", ";
			controller_tasks_types += ", ";
		}
		controller_names += "\"" + controller_ui_specs[0] + "\"";
		controller_tasks_names += controller_ui_specs[1];
		controller_tasks_types += controller_ui_specs[2];
	}

	controller_names += "]";
	controller_tasks_names += "]";
	controller_tasks_types += "]";
	return std::vector<std::string>{controller_names, controller_tasks_names,
									controller_tasks_types};
}

void MainRedisInterface::generateUiFile() {
	std::ifstream templateHtml(WEBUI_TEMPLATE_FILE_PATH);
	if (!templateHtml) {
		std::cerr << "Error: Unable to open template HTML file.\n";
		return;
	}

	// Read the content of the original HTML file
	std::string htmlContent((std::istreambuf_iterator<char>(templateHtml)),
							(std::istreambuf_iterator<char>()));

	// Close the original file
	templateHtml.close();

	std::string additionalContent;
	if (_controllers_configs.size() == 0) {
		// do nothing
	} else if (_controllers_configs.size() == 1) {
		std::vector<std::string> controller_names_and_tasks =
			generateControllerNamesAndTasksForUI(_controllers_configs[0]);
		additionalContent =
			"<div class='row mx-3'>\n<sai2-interfaces-robot-controller "
			"robotName='" +
			_controllers_configs[0].robot_name + "'\nredisPrefix='" +
			_controllers_configs[0].redis_prefix + "'\ncontrollerNames='" +
			controller_names_and_tasks[0] + "'\ncontrollerTaskNames='" +
			controller_names_and_tasks[1] + "'\ncontrollerTaskTypes='" +
			controller_names_and_tasks[2] + "' />\n</div>\n";
	} else {
		additionalContent += "<div class='row mx-3'>\n";
		additionalContent +=
			"<sai2-interfaces-tabs name='Robot_names' color='#b30000'>\n";

		for (const auto& config : _controllers_configs) {
			additionalContent += "<sai2-interfaces-tab-content name='" +
								 config.robot_name + "'>\n";

			std::vector<std::string> controller_names_and_tasks =
				generateControllerNamesAndTasksForUI(config);
			additionalContent +=
				"<div class='row my-3'>\n<sai2-interfaces-robot-controller "
				"robotName='" +
				config.robot_name + "'\nredisPrefix='" + config.redis_prefix +
				"'\ncontrollerNames='" + controller_names_and_tasks[0] +
				"'\ncontrollerTaskNames='" + controller_names_and_tasks[1] +
				"'\ncontrollerTaskTypes='" + controller_names_and_tasks[2] +
				"' />\n</div>\n";

			additionalContent += "</sai2-interfaces-tab-content>\n";
		}

		additionalContent += "</sai2-interfaces-tabs>\n";
		additionalContent += "</div>\n";
	}

	// Add content before the </body> tag
	size_t bodyPosition = htmlContent.find("</body>");
	if (bodyPosition != std::string::npos) {
		htmlContent.insert(bodyPosition, additionalContent);
	} else {
		std::cerr << "Error: </body> tag not found in template HTML file.\n";
		return;
	}

	// Write the modified content to a new file
	const std::string webui_generated_dir =
		_config_folder_path + "/webui_generated_file";
	std::filesystem::create_directories(webui_generated_dir);
	std::ofstream modifiedFile(webui_generated_dir + "/webui.html");
	if (!modifiedFile) {
		std::cerr << "Error: Unable to create modified HTML file.\n";
	}

	modifiedFile << htmlContent;

	// Close the modified file
	modifiedFile.close();

	std::cout << "Webui HTML file has been created successfully.\n";
}

void MainRedisInterface::runInterfaceLoop() {
	Sai2Common::RedisClient redis_client;
	redis_client.connect();

	redis_client.set(CONFIG_FILE_NAME_KEY, _config_file_name);
	redis_client.setBool(RESET_KEY, true);

	while (!::external_stop_signal) {
		std::string new_config_file_name =
			redis_client.get(CONFIG_FILE_NAME_KEY);

		if (_config_file_name != new_config_file_name ||
			redis_client.getBool(RESET_KEY)) {
			bool succes = parseConfig(new_config_file_name);
			generateUiFile();
			if (succes) {
				reset();
			}
			redis_client.setBool(RESET_KEY, false);
			redis_client.set(CONFIG_FILE_NAME_KEY, _config_file_name);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	::controllers_stop_signal = true;
	::simviz_stop_signal = true;
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
		::simviz_stop_signal = true;
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
	::controllers_stop_signal = false;
	_controllers_threads.clear();
	for (const auto& config : _controllers_configs) {
		_controllers_interfaces[config.robot_name] =
			make_unique<RobotControllerRedisInterface>(config, false);
		_controllers_threads.push_back(std::thread([&]() {
			_controllers_interfaces.at(config.robot_name)
				->run(::controllers_stop_signal);
		}));
	}
}

void MainRedisInterface::stopRunningControllers() {
	::controllers_stop_signal = true;
	for (auto& thread : _controllers_threads) {
		thread.join();
	}
	_controllers_threads.clear();
	_controllers_interfaces.clear();
}

}  // namespace Sai2Interfaces