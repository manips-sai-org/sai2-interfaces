#include "RobotControllerConfigParser.h"

#include <urdf/urdfdom_headers/urdf_model/include/urdf_model/pose.h>

#include <iostream>

using namespace std;
using namespace Eigen;

namespace Sai2Interfaces {

namespace {

bool parsePose(Sai2Urdfreader::Pose& pose, tinyxml2::XMLElement* xml) {
	pose.clear();
	if (xml) {
		const char* xyz_str = xml->Attribute("xyz");
		if (xyz_str != NULL) {
			try {
				pose.position.init(xyz_str);
			} catch (std::exception e) {
				std::cout << e.what() << std::endl;
				return false;
			}
		}

		const char* rpy_str = xml->Attribute("rpy");
		if (rpy_str != NULL) {
			try {
				pose.rotation.init(rpy_str);
			} catch (std::exception e) {
				std::cout << e.what() << std::endl;
				return false;
			}
		}
	}
	return true;
}

Affine3d parsePoseLocal(tinyxml2::XMLElement* xml) {
	Affine3d pose = Affine3d::Identity();

	Sai2Urdfreader::Pose pose_urdf;
	parsePose(pose_urdf, xml);

	pose.translation() << pose_urdf.position.x, pose_urdf.position.y,
		pose_urdf.position.z;
	pose.linear() = Quaterniond(pose_urdf.rotation.w, pose_urdf.rotation.x,
								pose_urdf.rotation.y, pose_urdf.rotation.z)
						.toRotationMatrix();

	return pose;
}

Vector3d parseVector3dLocal(tinyxml2::XMLElement* xml,
							string attribute_name = "xyz") {
	Sai2Urdfreader::Vector3 vec;

	const char* xyz_str = xml->Attribute(attribute_name.c_str());
	if (xyz_str != NULL) {
		try {
			vec.init(xyz_str);
		} catch (Sai2Urdfreader::ParseError& e) {
			cout << e.what() << endl;
			throw runtime_error("Could not parse vector3d");
		}
	}

	return Vector3d(vec.x, vec.y, vec.z);
}

Vector3d parseVector3dLocal(const char* xyz_str) {
	Sai2Urdfreader::Vector3 vec;

	if (xyz_str != NULL) {
		try {
			vec.init(xyz_str);
		} catch (Sai2Urdfreader::ParseError& e) {
			cout << e.what() << endl;
			throw runtime_error("Could not parse vector3d");
		}
	}

	return Vector3d(vec.x, vec.y, vec.z);
}

std::vector<std::string> splitString(const std::string& str,
									 const std::vector<char>& separators = {
										 ' ', '\t', '\n', ','}) {
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string token;

	while (std::getline(iss, token)) {
		std::string current;
		bool inToken = false;

		for (char ch : token) {
			if (std::find(separators.begin(), separators.end(), ch) !=
				separators.end()) {
				if (inToken) {
					tokens.push_back(current);
					current.clear();
					inToken = false;
				}
			} else {
				current += ch;
				inToken = true;
			}
		}

		if (inToken) {
			tokens.push_back(current);
		}
	}

	return tokens;
}

enum GainsType {
	JOINT_GAINS,
	MOTFORCE_POS,
	MOTFORCE_ORI,
	MOTFORCE_FORCE,
	MOTFORCE_MOMENT
};

GainsConfig parseGainsConfig(tinyxml2::XMLElement* xml,
							 const string& config_file_name,
							 const GainsType gains_type) {
	string gains_name;
	double default_kp, default_kv, default_ki;

	switch (gains_type) {
		case JOINT_GAINS:
			gains_name = "jointGains";
			default_kp = JointTaskDefaultParams::kp;
			default_kv = JointTaskDefaultParams::kv;
			default_ki = JointTaskDefaultParams::ki;
			break;
		case MOTFORCE_POS:
			gains_name = "positionGains";
			default_kp = MotionForceTaskDefaultParams::kp_pos;
			default_kv = MotionForceTaskDefaultParams::kv_pos;
			default_ki = MotionForceTaskDefaultParams::ki_pos;
			break;
		case MOTFORCE_ORI:
			gains_name = "orientationGains";
			default_kp = MotionForceTaskDefaultParams::kp_ori;
			default_kv = MotionForceTaskDefaultParams::kv_ori;
			default_ki = MotionForceTaskDefaultParams::ki_ori;
			break;
		case MOTFORCE_FORCE:
			gains_name = "forceGains";
			default_kp = MotionForceTaskDefaultParams::kp_force;
			default_kv = MotionForceTaskDefaultParams::kv_force;
			default_ki = MotionForceTaskDefaultParams::ki_force;
			break;
		case MOTFORCE_MOMENT:
			gains_name = "momentGains";
			default_kp = MotionForceTaskDefaultParams::kp_moment;
			default_kv = MotionForceTaskDefaultParams::kv_moment;
			default_ki = MotionForceTaskDefaultParams::ki_moment;
			break;
	}

	GainsConfig gains_config = GainsConfig(default_kp, default_kv, default_ki);

	const char* kp = xml->Attribute("kp");
	const char* kv = xml->Attribute("kv");
	const char* ki = xml->Attribute("ki");

	vector<string> vectorKp = {};
	vector<string> vectorKv = {};
	vector<string> vectorKi = {};

	if (kp) {
		vectorKp = splitString(kp);
	}
	if (kv) {
		vectorKv = splitString(kv);
	}
	if (ki) {
		vectorKi = splitString(ki);
	}

	const int size =
		max(vectorKp.size(), max(vectorKv.size(), vectorKi.size()));

	if (size == 0) {
		return gains_config;
	}

	if (size == 1) {
		if (vectorKp.size() == 1) {
			gains_config.kp.setConstant(1, stod(vectorKp[0]));
		}
		if (vectorKv.size() == 1) {
			gains_config.kv.setConstant(1, stod(vectorKv[0]));
		}
		if (vectorKi.size() == 1) {
			gains_config.ki.setConstant(1, stod(vectorKi[0]));
		}
		return gains_config;
	}

	if (vectorKp.size() > 1 && vectorKp.size() != size ||
		vectorKv.size() > 1 && vectorKv.size() != size ||
		vectorKi.size() > 1 && vectorKi.size() != size) {
		throw runtime_error(
			gains_name +
			" gains must have the same number of kp, kv and ki values for "
			"those in vector form in config file: " +
			config_file_name);
	}

	if (gains_type == MOTFORCE_FORCE || gains_type == MOTFORCE_MOMENT) {
		throw runtime_error(gains_name +
							" gains must have 1 kp, kv or ki value if present "
							"in config file: " +
							config_file_name);
	}

	if ((gains_type == MOTFORCE_POS || gains_type == MOTFORCE_ORI) &&
		size != 3) {
		throw runtime_error(gains_name +
							" gains must have 3 kp, kv or ki values for "
							"those in vector form in config file: " +
							config_file_name);
	}

	if (vectorKp.size() == 0) {
		gains_config.kp.setConstant(size, default_kp);
	} else if (vectorKp.size() == 1) {
		gains_config.kp.setConstant(size, stod(vectorKp[0]));
	} else {
		VectorXd kp_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			kp_vec[i] = stod(vectorKp[i]);
		}
		gains_config.kp = kp_vec;
	}

	if (vectorKv.size() == 0) {
		gains_config.kv.setConstant(size, default_kv);
	} else if (vectorKv.size() == 1) {
		gains_config.kv.setConstant(size, stod(vectorKv[0]));
	} else {
		VectorXd kv_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			kv_vec[i] = stod(vectorKv[i]);
		}
		gains_config.kv = kv_vec;
	}

	if (vectorKi.size() == 0) {
		gains_config.ki.setConstant(size, default_ki);
	} else if (vectorKi.size() == 1) {
		gains_config.ki.setConstant(size, stod(vectorKi[0]));
	} else {
		VectorXd ki_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			ki_vec[i] = stod(vectorKi[i]);
		}
		gains_config.ki = ki_vec;
	}

	return gains_config;
}

JointTaskConfig::JointOTGConfig parseOTGJointConfig(
	tinyxml2::XMLElement* otg_xml, const string& config_file_name) {
	JointTaskConfig::JointOTGConfig otg_config;

	// type
	const char* type = otg_xml->Attribute("type");
	if (!type) {
		throw runtime_error("otg must have a type in config file: " +
							config_file_name);
	}
	if (string(type) == "disabled") {
		otg_config.enabled = false;
	} else if (string(type) == "acceleration") {
		otg_config.enabled = true;
		otg_config.jerk_limited = false;
	} else if (string(type) == "jerk") {
		otg_config.enabled = true;
		otg_config.jerk_limited = true;
	} else {
		throw runtime_error("Unknown otg type: " + string(type));
	}

	const char* velocity_limit = otg_xml->Attribute("max_velocity");
	const char* acceleration_limit = otg_xml->Attribute("max_acceleration");
	const char* jerk_limit = otg_xml->Attribute("max_jerk");

	vector<string> vectorVelocityLimit = {};
	vector<string> vectorAccelerationLimit = {};
	vector<string> vectorJerkLimit = {};

	if (velocity_limit) {
		vectorVelocityLimit = splitString(velocity_limit);
	}
	if (acceleration_limit) {
		vectorAccelerationLimit = splitString(acceleration_limit);
	}
	if (jerk_limit) {
		vectorJerkLimit = splitString(jerk_limit);
	}

	const int size =
		max(vectorVelocityLimit.size(),
			max(vectorAccelerationLimit.size(), vectorJerkLimit.size()));

	if (size == 0) {
		return otg_config;
	}

	if (size == 1) {
		if (vectorVelocityLimit.size() == 1) {
			otg_config.limits.velocity_limit.setConstant(
				1, stod(vectorVelocityLimit[0]));
		}
		if (vectorAccelerationLimit.size() == 1) {
			otg_config.limits.acceleration_limit.setConstant(
				1, stod(vectorAccelerationLimit[0]));
		}
		if (vectorJerkLimit.size() == 1) {
			otg_config.limits.jerk_limit.setConstant(1,
													 stod(vectorJerkLimit[0]));
		}
		return otg_config;
	}

	if (vectorVelocityLimit.size() > 1 && vectorVelocityLimit.size() != size ||
		vectorAccelerationLimit.size() > 1 &&
			vectorAccelerationLimit.size() != size ||
		vectorJerkLimit.size() > 1 && vectorJerkLimit.size() != size) {
		throw runtime_error(
			"otg config limits must have the same number of max_velocity, "
			"max_acceleration, and max_jerk values for those in vector form in "
			"config file: " +
			config_file_name);
	}

	if (vectorVelocityLimit.size() == 0) {
		otg_config.limits.velocity_limit.setConstant(
			size, JointTaskDefaultParams::otg_max_velocity);
	} else if (vectorVelocityLimit.size() == 1) {
		otg_config.limits.velocity_limit.setConstant(
			size, stod(vectorVelocityLimit[0]));
	} else {
		VectorXd vel_limits_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			vel_limits_vec[i] = stod(vectorVelocityLimit[i]);
		}
		otg_config.limits.velocity_limit = vel_limits_vec;
	}

	if (vectorAccelerationLimit.size() == 0) {
		otg_config.limits.acceleration_limit.setConstant(
			size, JointTaskDefaultParams::otg_max_acceleration);
	} else if (vectorAccelerationLimit.size() == 1) {
		otg_config.limits.acceleration_limit.setConstant(
			size, stod(vectorAccelerationLimit[0]));
	} else {
		VectorXd acc_limits_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			acc_limits_vec[i] = stod(vectorAccelerationLimit[i]);
		}
		otg_config.limits.acceleration_limit = acc_limits_vec;
	}

	if (vectorJerkLimit.size() == 0) {
		otg_config.limits.jerk_limit.setConstant(
			size, JointTaskDefaultParams::otg_max_jerk);
	} else if (vectorJerkLimit.size() == 1) {
		otg_config.limits.jerk_limit.setConstant(size,
												 stod(vectorJerkLimit[0]));
	} else {
		VectorXd jerk_limits_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			jerk_limits_vec[i] = stod(vectorJerkLimit[i]);
		}
		otg_config.limits.jerk_limit = jerk_limits_vec;
	}

	return otg_config;
}

JointTaskConfig::JointVelSatConfig parseVelSatJointConfig(
	tinyxml2::XMLElement* vel_sat_xml, const string& config_file_name) {
	JointTaskConfig::JointVelSatConfig vel_sat_config;

	// enabled
	if (!vel_sat_xml->Attribute("enabled")) {
		throw runtime_error(
			"velocitySaturation must have an enabled attribute if present in "
			"joint task in config file: " +
			config_file_name);
	}
	vel_sat_config.enabled = vel_sat_xml->BoolAttribute("enabled");

	const char* velocity_limits = vel_sat_xml->Attribute("velocity_limit");
	if (velocity_limits) {
		vector<string> vectorVelocityLimits = splitString(velocity_limits);
		const int size = vectorVelocityLimits.size();
		VectorXd vel_limits_vec = VectorXd(size);
		for (int i = 0; i < size; i++) {
			vel_limits_vec[i] = stod(vectorVelocityLimits[i]);
		}
		vel_sat_config.velocity_limits = vel_limits_vec;
	}
	return vel_sat_config;
}

}  // namespace

std::vector<RobotControllerConfig> RobotControllerConfigParser::parseConfig(
	const std::string& config_file) {
	_config_file_name = config_file;
	std::vector<RobotControllerConfig> configs;

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(config_file.c_str()) != tinyxml2::XML_SUCCESS) {
		throw runtime_error("Could not load controller config file: " +
							config_file);
	}

	if (doc.FirstChildElement("robotControlConfiguration") == nullptr) {
		throw runtime_error(
			"No 'robotControlConfiguration' element found in config file: " +
			config_file);
	}

	// loop over the robotControlConfiguration elements
	for (tinyxml2::XMLElement* robotControlConfiguration =
			 doc.FirstChildElement("robotControlConfiguration");
		 robotControlConfiguration;
		 robotControlConfiguration =
			 robotControlConfiguration->NextSiblingElement(
				 "robotControlConfiguration")) {
		// get the robot name
		if (!robotControlConfiguration->Attribute("robotName")) {
			throw runtime_error(
				"Some of the robotControlConfiguration are missing a "
				"'robotName' attribute field in config file: " +
				config_file);
		}
		const std::string robot_name =
			robotControlConfiguration->Attribute("robotName");

		// if (robotControlConfiguration->Attribute("file")) {
		// 	const std::string internal_controller_config_file =
		// 		robotControlConfiguration->Attribute("file");
		// 	tinyxml2::XMLDocument doc_internal;
		// 	if (doc_internal.LoadFile(
		// 			internal_controller_config_file.c_str()) !=
		// 		tinyxml2::XML_SUCCESS) {
		// 		throw runtime_error("Could not load controller config file: " +
		// 							internal_controller_config_file);
		// 	}
		// 	if (doc_internal.FirstChildElement("robotControlConfiguration") ==
		// 		nullptr) {
		// 		throw runtime_error(
		// 			"no 'robotControlConfiguration' element found in config "
		// 			"file " +
		// 			internal_controller_config_file);
		// 	}
		// 	configs.push_back(parseControllersConfig(
		// 		doc_internal.FirstChildElement("robotControlConfiguration")));
		// } else {
		configs.push_back(parseControllersConfig(robotControlConfiguration));
		// }
		configs.back().robot_name = robot_name;

		// get the redis prefix
		if (robotControlConfiguration->Attribute("redisPrefix")) {
			configs.back().redis_prefix =
				robotControlConfiguration->Attribute("redisPrefix");
		}

		// get the controller frequency
		if (robotControlConfiguration->Attribute("controlFrequency")) {
			configs.back().control_frequency =
				robotControlConfiguration->DoubleAttribute("controlFrequency");
		}
	}

	return configs;
}

RobotControllerConfig RobotControllerConfigParser::parseControllersConfig(
	tinyxml2::XMLElement* controlConfiguration) {
	RobotControllerConfig config;

	// Extract the robotModelFile
	tinyxml2::XMLElement* robotModelFile =
		controlConfiguration->FirstChildElement("robotModelFile");
	if (!robotModelFile) {
		throw runtime_error(
			"No 'robotModelFile' element found in config file: " +
			_config_file_name);
	}
	config.robot_model_file = robotModelFile->GetText();

	// robot base in world
	tinyxml2::XMLElement* robotBaseInWorld =
		controlConfiguration->FirstChildElement("robotBaseInWorld");
	if (robotBaseInWorld) {
		config.robot_base_in_world = parsePoseLocal(robotBaseInWorld);
	}

	// world gravity
	tinyxml2::XMLElement* worldGravity =
		controlConfiguration->FirstChildElement("worldGravity");
	if (worldGravity) {
		config.world_gravity = parseVector3dLocal(worldGravity);
	}

	// extract logger config
	tinyxml2::XMLElement* logger =
		controlConfiguration->FirstChildElement("logger");
	if (logger) {
		if (logger->FirstChildElement("logFolderName")) {
			config.logger_config.folder_name =
				logger->FirstChildElement("logFolderName")->GetText();
		}

		if (logger->FirstChildElement("logFrequency")) {
			config.logger_config.frequency =
				logger->FirstChildElement("logFrequency")->DoubleText();
		}

		if (logger->FirstChildElement("startWithController")) {
			config.logger_config.start_with_logger_on =
				logger->FirstChildElement("startWithController")->BoolText();
		}

		if (logger->FirstChildElement("timestampInFilename")) {
			config.logger_config.add_timestamp_to_filename =
				logger->FirstChildElement("timestampInFilename")->BoolText();
		}
	}

	// parse all controller configs
	for (tinyxml2::XMLElement* controller =
			 controlConfiguration->FirstChildElement("controller");
		 controller;
		 controller = controller->NextSiblingElement("controller")) {
		// get controller name
		const char* name = controller->Attribute("name");
		if (!name) {
			throw runtime_error(
				"controllers must have a name in config file: " +
				_config_file_name);
		}
		if (name == string("")) {
			throw runtime_error(
				"controllers must have a non-empty name in config file: " +
				_config_file_name);
		}
		if (config.controllers_configs.find(name) !=
			config.controllers_configs.end()) {
			throw runtime_error(
				"controllers must have a unique name in config file: " +
				_config_file_name);
		}

		config.controllers_configs[name] =
			parseSingleControllerConfig(controller);

		if (config.initial_active_controller_name == "") {
			config.initial_active_controller_name = name;
		}
	}
	return config;
}

vector<variant<JointTaskConfig, MotionForceTaskConfig>>
RobotControllerConfigParser::parseSingleControllerConfig(
	tinyxml2::XMLElement* xml) {
	vector<variant<JointTaskConfig, MotionForceTaskConfig>> configs;

	vector<string> controller_task_names;

	// loop over tasks
	for (tinyxml2::XMLElement* task = xml->FirstChildElement("task"); task;
		 task = task->NextSiblingElement("task")) {
		// get task type
		const char* type = task->Attribute("type");
		if (!type) {
			throw runtime_error("tasks must have a type in config file: " +
								_config_file_name);
		}
		if (string(type) == "joint_task") {
			configs.push_back(parseJointTaskConfig(task));
			if (find(controller_task_names.begin(), controller_task_names.end(),
					 get<JointTaskConfig>(configs.back()).task_name) !=
				controller_task_names.end()) {
				throw runtime_error(
					"tasks from the same controller must have a unique name in "
					"config file: " +
					_config_file_name);
			}
		} else if (string(type) == "motion_force_task") {
			configs.push_back(parseMotionForceTaskConfig(task));
			if (find(controller_task_names.begin(), controller_task_names.end(),
					 get<MotionForceTaskConfig>(configs.back()).task_name) !=
				controller_task_names.end()) {
				throw runtime_error(
					"tasks from the same controller must have a unique name in "
					"config file: " +
					_config_file_name);
			}
		} else {
			throw runtime_error("Unknown task type: " + string(type));
		}
	}
	return configs;
}

JointTaskConfig RobotControllerConfigParser::parseJointTaskConfig(
	tinyxml2::XMLElement* xml) {
	JointTaskConfig config;
	// get name
	const char* name = xml->Attribute("name");
	if (!name) {
		throw runtime_error("tasks must have a name in config file: " +
							_config_file_name);
	}
	if (string(name) == "") {
		throw runtime_error(
			"tasks must have a non-empty name in config file: " +
			_config_file_name);
	}
	config.task_name = name;

	// get controlled joints
	tinyxml2::XMLElement* controlled_joints =
		xml->FirstChildElement("controlledJointNames");
	if (controlled_joints && controlled_joints->GetText()) {
		config.controlled_joint_names =
			splitString(controlled_joints->GetText());
	}

	// dynamic decoupling
	tinyxml2::XMLElement* dynamic_decoupling =
		xml->FirstChildElement("dynamicDecoupling");
	if (dynamic_decoupling) {
		config.use_dynamic_decoupling = dynamic_decoupling->BoolText();
	}

	// velocity saturation
	tinyxml2::XMLElement* velocity_saturation =
		xml->FirstChildElement("velocitySaturation");
	if (velocity_saturation) {
		config.velocity_saturation_config =
			parseVelSatJointConfig(velocity_saturation, _config_file_name);
	}

	// otg
	tinyxml2::XMLElement* otg = xml->FirstChildElement("otg");
	if (otg) {
		config.otg_config = parseOTGJointConfig(otg, _config_file_name);
	}

	// gains
	tinyxml2::XMLElement* gains = xml->FirstChildElement("gains");
	if (gains) {
		config.gains_config =
			parseGainsConfig(gains, _config_file_name, JOINT_GAINS);
	}

	return config;
}

MotionForceTaskConfig RobotControllerConfigParser::parseMotionForceTaskConfig(
	tinyxml2::XMLElement* xml) {
	MotionForceTaskConfig config;
	// get name
	const char* name = xml->Attribute("name");
	if (!name) {
		throw runtime_error("tasks must have a name in config file: " +
							_config_file_name);
	}
	if (string(name) == "") {
		throw runtime_error(
			"tasks must have a non-empty name in config file: " +
			_config_file_name);
	}
	config.task_name = name;

	// link name
	const char* link_name = xml->FirstChildElement("linkName")->GetText();
	if (!link_name) {
		throw runtime_error("tasks must have a linkName in config file: " +
							_config_file_name);
	}
	config.link_name = link_name;

	// compliant frame
	tinyxml2::XMLElement* compliant_frame =
		xml->FirstChildElement("compliantFrame");
	if (compliant_frame) {
		config.compliant_frame = parsePoseLocal(compliant_frame);
	}

	// parametrization in compliant frame
	tinyxml2::XMLElement* parametrization_in_compliant_frame =
		xml->FirstChildElement("parametrizationInCompliantFrame");
	if (parametrization_in_compliant_frame) {
		config.is_parametrization_in_compliant_frame =
			parametrization_in_compliant_frame->BoolText();
	}

	// use dynamic decoupling
	tinyxml2::XMLElement* dynamic_decoupling =
		xml->FirstChildElement("dynamicDecoupling");
	if (dynamic_decoupling) {
		config.use_dynamic_decoupling = dynamic_decoupling->BoolText();
	}

	// controlled directions position
	tinyxml2::XMLElement* controlled_directions_position =
		xml->FirstChildElement("controlledDirectionsTranslation");
	if (controlled_directions_position) {
		vector<Vector3d> controlled_directions;
		for (tinyxml2::XMLElement* direction =
				 controlled_directions_position->FirstChildElement("direction");
			 direction;
			 direction = direction->NextSiblingElement("direction")) {
			controlled_directions.push_back(parseVector3dLocal(direction));
		}
		config.controlled_directions_position = controlled_directions;
	}

	// controlled directions orientation
	tinyxml2::XMLElement* controlled_directions_orientation =
		xml->FirstChildElement("controlledDirectionsRotation");
	if (controlled_directions_orientation) {
		vector<Vector3d> controlled_directions;
		for (tinyxml2::XMLElement* direction =
				 controlled_directions_orientation->FirstChildElement(
					 "direction");
			 direction;
			 direction = direction->NextSiblingElement("direction")) {
			controlled_directions.push_back(parseVector3dLocal(direction));
		}
		config.controlled_directions_orientation = controlled_directions;
	}

	// closed looop force control
	tinyxml2::XMLElement* closed_loop_force_control =
		xml->FirstChildElement("closedLoopForceControl");
	if (closed_loop_force_control) {
		config.closed_loop_force_control =
			closed_loop_force_control->BoolText();
	}

	// force sensor frame
	tinyxml2::XMLElement* force_sensor_frame =
		xml->FirstChildElement("forceSensorFrame");
	if (force_sensor_frame) {
		config.force_sensor_frame = parsePoseLocal(force_sensor_frame);
	} else {
		config.force_sensor_frame = config.compliant_frame;
	}

	// force space param
	tinyxml2::XMLElement* force_space_param =
		xml->FirstChildElement("forceSpaceParametrization");
	if (force_space_param) {
		MotionForceTaskConfig::ForceMotionSpaceParamConfig
			force_motion_space_config;
		if (force_space_param->Attribute("dim")) {
			force_motion_space_config.force_space_dimension =
				force_space_param->IntAttribute("dim");
		}
		if (force_space_param->Attribute("direction")) {
			force_motion_space_config.axis =
				parseVector3dLocal(force_space_param->Attribute("direction"));
			if (force_motion_space_config.axis.norm() > 1e-3) {
				force_motion_space_config.axis.normalize();
			}
		}
		config.force_space_param_config = force_motion_space_config;
	}

	// moment space param
	tinyxml2::XMLElement* moment_space_param =
		xml->FirstChildElement("momentSpaceParametrization");
	if (moment_space_param) {
		MotionForceTaskConfig::ForceMotionSpaceParamConfig
			moment_space_param_config;
		if (moment_space_param->Attribute("dim")) {
			moment_space_param_config.force_space_dimension =
				moment_space_param->IntAttribute("dim");
		}
		if (moment_space_param->Attribute("direction")) {
			moment_space_param_config.axis =
				parseVector3dLocal(moment_space_param->Attribute("direction"));
			if (moment_space_param_config.axis.norm() > 1e-3) {
				moment_space_param_config.axis.normalize();
			}
		}
		config.moment_space_param_config = moment_space_param_config;
	}

	// velocity saturation
	tinyxml2::XMLElement* velocity_saturation =
		xml->FirstChildElement("velocitySaturation");
	if (velocity_saturation) {
		MotionForceTaskConfig::VelSatConfig vel_sat_config;
		// enabled
		if (!velocity_saturation->Attribute("enabled")) {
			throw runtime_error(
				"velocitySaturation must have an enabled attribute if present "
				"in "
				"MotionForceTask in config file: " +
				_config_file_name);
		}
		vel_sat_config.enabled = velocity_saturation->BoolAttribute("enabled");

		const char* linear_velocity_limits =
			velocity_saturation->Attribute("linear_velocity_limit");
		if (linear_velocity_limits) {
			vel_sat_config.linear_velocity_limits =
				stod(linear_velocity_limits);
		}

		const char* angular_velocity_limits =
			velocity_saturation->Attribute("angular_velocity_limit");
		if (angular_velocity_limits) {
			vel_sat_config.angular_velocity_limits =
				stod(angular_velocity_limits);
		}
		config.velocity_saturation_config = vel_sat_config;
	}

	// otg
	tinyxml2::XMLElement* otg = xml->FirstChildElement("otg");
	if (otg) {
		MotionForceTaskConfig::OTGConfig otg_config;
		// type
		const char* type = otg->Attribute("type");
		if (!type) {
			throw runtime_error("otg must have a type in config file: " +
								_config_file_name);
		}
		if (string(type) == "disabled") {
			otg_config.enabled = false;
		} else if (string(type) == "acceleration") {
			otg_config.enabled = true;
			otg_config.jerk_limited = false;
		} else if (string(type) == "jerk") {
			otg_config.enabled = true;
			otg_config.jerk_limited = true;
		} else {
			throw runtime_error("Unknown otg type: " + string(type) +
								"in MotionForce task config in config file: " +
								_config_file_name);
		}

		// velocity limits
		const char* linear_velocity_limits =
			otg->Attribute("max_linear_velocity");
		if (linear_velocity_limits) {
			otg_config.linear_velocity_limit = stod(linear_velocity_limits);
		}

		const char* angular_velocity_limits =
			otg->Attribute("max_angular_velocity");
		if (angular_velocity_limits) {
			otg_config.angular_velocity_limit = stod(angular_velocity_limits);
		}

		// acceleration limit
		const char* linear_acceleration_limit =
			otg->Attribute("max_linear_acceleration");
		if (linear_acceleration_limit) {
			otg_config.linear_acceleration_limit =
				stod(linear_acceleration_limit);
		}

		const char* angular_acceleration_limit =
			otg->Attribute("max_angular_acceleration");
		if (angular_acceleration_limit) {
			otg_config.angular_acceleration_limit =
				stod(angular_acceleration_limit);
		}

		// jerk limit
		const char* linear_jerk_limit = otg->Attribute("max_linear_jerk");
		if (linear_jerk_limit) {
			otg_config.linear_jerk_limit = stod(linear_jerk_limit);
		}

		const char* angular_jerk_limit = otg->Attribute("max_angular_jerk");
		if (angular_jerk_limit) {
			otg_config.angular_jerk_limit = stod(angular_jerk_limit);
		}
		config.otg_config = otg_config;
	}

	// position gains
	tinyxml2::XMLElement* position_gains =
		xml->FirstChildElement("positionGains");
	if (position_gains) {
		config.position_gains_config =
			parseGainsConfig(position_gains, _config_file_name, MOTFORCE_POS);
	}

	// orientation gains
	tinyxml2::XMLElement* orientation_gains =
		xml->FirstChildElement("orientationGains");
	if (orientation_gains) {
		config.orientation_gains_config = parseGainsConfig(
			orientation_gains, _config_file_name, MOTFORCE_ORI);
	}

	// force gains
	tinyxml2::XMLElement* force_gains = xml->FirstChildElement("forceGains");
	if (force_gains) {
		config.force_gains_config =
			parseGainsConfig(force_gains, _config_file_name, MOTFORCE_FORCE);
	}

	// moment gains
	tinyxml2::XMLElement* moment_gains = xml->FirstChildElement("momentGains");
	if (moment_gains) {
		config.moment_gains_config =
			parseGainsConfig(moment_gains, _config_file_name, MOTFORCE_MOMENT);
	}

	return config;
}

}  // namespace Sai2Interfaces