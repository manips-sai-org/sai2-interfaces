#include "RobotControllerConfigParser.h"

#include <iostream>
#include <urdf/urdfdom/urdf_parser/src/pose.cpp>

namespace Sai2Interfaces {

namespace {

Eigen::Affine3d parsePoseLocal(tinyxml2::XMLElement* xml) {
	Eigen::Affine3d pose = Eigen::Affine3d::Identity();

	Sai2Urdfreader::Pose pose_urdf;
	Sai2Urdfreader::parsePose(pose_urdf, xml);

	pose.translation() << pose_urdf.position.x, pose_urdf.position.y,
		pose_urdf.position.z;
	pose.linear() =
		Eigen::Quaterniond(pose_urdf.rotation.w, pose_urdf.rotation.x,
						   pose_urdf.rotation.y, pose_urdf.rotation.z)
			.toRotationMatrix();

	return pose;
}

Eigen::Vector3d parseVector3dLocal(tinyxml2::XMLElement* xml,
								   std::string attribute_name = "xyz") {
	Sai2Urdfreader::Vector3 vec;

	const char* xyz_str = xml->Attribute(attribute_name.c_str());
	if (xyz_str != NULL) {
		try {
			vec.init(xyz_str);
		} catch (Sai2Urdfreader::ParseError& e) {
			std::cout << e.what() << std::endl;
			throw std::runtime_error("Could not parse vector3d");
		}
	}

	return Eigen::Vector3d(vec.x, vec.y, vec.z);
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string token;

	while (std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

GainsConfig parseGainsConfig(tinyxml2::XMLElement* xml,
							 const std::string& config_file_name,
							 const std::string& gains_name,
							 const bool is_cartesian_gains) {
	const char* kp = xml->Attribute("kp");
	const char* kv = xml->Attribute("kv");
	const char* ki = xml->Attribute("ki");
	if (!kp || !kv || !ki) {
		throw std::runtime_error(
			gains_name +
			" must have a kp, kv and ki in config file: " + config_file_name);
	}

	std::vector<std::string> vectorKp = splitString(kp, ' ');
	std::vector<std::string> vectorKv = splitString(kv, ' ');
	std::vector<std::string> vectorKi = splitString(ki, ' ');

	if (vectorKv.size() != vectorKp.size() ||
		vectorKi.size() != vectorKp.size()) {
		throw std::runtime_error(gains_name +
								 " gains must have the same number of kp, kv "
								 "and ki values in config file: " +
								 config_file_name);
	}
	const int size = vectorKp.size();

	if (size < 1) {
		throw std::runtime_error(gains_name +
								 " gains must have at least one kp, kv "
								 "and ki value in config file: " +
								 config_file_name);
	}

	if (is_cartesian_gains && size != 1 && size != 3) {
		throw std::runtime_error(gains_name +
								 " gains must have 1 or 3 kp, kv "
								 "and ki values in config file: " +
								 config_file_name);
	}

	GainsConfig gains_config;
	Eigen::VectorXd kp_vec = Eigen::VectorXd(size);
	Eigen::VectorXd kv_vec = Eigen::VectorXd(size);
	Eigen::VectorXd ki_vec = Eigen::VectorXd(size);
	for (int i = 0; i < size; i++) {
		kp_vec[i] = std::stod(vectorKp[i]);
		kv_vec[i] = std::stod(vectorKv[i]);
		ki_vec[i] = std::stod(vectorKi[i]);
	}
	gains_config.kp = kp_vec;
	gains_config.kv = kv_vec;
	gains_config.ki = ki_vec;
	return gains_config;
}

JointTaskConfig::JointOTGConfig parseOTGJointConfig(
	tinyxml2::XMLElement* otg_xml, const std::string& config_file_name) {
	JointTaskConfig::JointOTGConfig otg_config;

	// type
	const char* type = otg_xml->Attribute("type");
	if (!type) {
		throw std::runtime_error("otg must have a type in config file: " +
								 config_file_name);
	}
	if (std::string(type) == "disabled") {
		otg_config.enabled = false;
		return otg_config;
	} else if (std::string(type) == "acceleration") {
		otg_config.enabled = true;
		otg_config.jerk_limited = false;
	} else if (std::string(type) == "jerk") {
		otg_config.enabled = true;
		otg_config.jerk_limited = true;
	} else {
		throw std::runtime_error("Unknown otg type: " + std::string(type));
	}

	const char* velocity_limits = otg_xml->Attribute("max_velocity");
	const char* acceleration_limit = otg_xml->Attribute("max_acceleration");
	const char* jerk_limit = otg_xml->Attribute("max_jerk");

	if (otg_config.enabled && (!velocity_limits || !acceleration_limit)) {
		throw std::runtime_error(
			"otg must have a max_velocity and max_acceleration if not disabled "
			"in config file: " +
			config_file_name);
	}
	if (otg_config.enabled && otg_config.jerk_limited && !jerk_limit) {
		throw std::runtime_error(
			"otg must have a max_jerk if jerk limited in config file: " +
			config_file_name);
	}

	std::vector<std::string> vectorVelocityLimits =
		splitString(velocity_limits, ' ');
	std::vector<std::string> vectorAccelerationLimit =
		splitString(acceleration_limit, ' ');
	std::vector<std::string> vectorJerkLimit = {};

	if (vectorVelocityLimits.size() != vectorAccelerationLimit.size()) {
		throw std::runtime_error(
			"otg must have the same number of max_velocity and "
			"max_acceleration values in config file: " +
			config_file_name);
	}
	if (jerk_limit) {
		vectorJerkLimit = splitString(jerk_limit, ' ');
		if (vectorVelocityLimits.size() != vectorJerkLimit.size()) {
			throw std::runtime_error(
				"otg must have the same number of max_velocity and max_jerk "
				"values in config file: " +
				config_file_name);
		}
	}
	const int size = vectorVelocityLimits.size();

	JointTaskConfig::JointOTGConfig::JointOTGLimit limits;
	Eigen::VectorXd vel_limits_vec = Eigen::VectorXd(size);
	Eigen::VectorXd acc_limits_vec = Eigen::VectorXd(size);
	for (int i = 0; i < size; i++) {
		vel_limits_vec[i] = std::stod(vectorVelocityLimits[i]);
		acc_limits_vec[i] = std::stod(vectorAccelerationLimit[i]);
	}
	limits.velocity_limit = vel_limits_vec;
	limits.acceleration_limit = acc_limits_vec;
	if (otg_config.enabled && otg_config.jerk_limited) {
		Eigen::VectorXd jerk_limits_vec = Eigen::VectorXd(size);
		for (int i = 0; i < size; i++) {
			jerk_limits_vec[i] = std::stod(vectorJerkLimit[i]);
		}
		limits.jerk_limit = jerk_limits_vec;
	}
	otg_config.limits = limits;
	return otg_config;
}

}  // namespace

RobotControllerConfig RobotControllerConfigParser::parseConfig(
	const std::string& config_file) {
	_config_file_name = config_file;
	RobotControllerConfig config;

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(config_file.c_str()) != tinyxml2::XML_SUCCESS) {
		throw std::runtime_error("Could not load simviz config file: " +
								 config_file);
	}

	tinyxml2::XMLElement* root = doc.FirstChildElement("controlConfiguration");
	if (!root) {
		throw std::runtime_error(
			"No 'controlConfiguration' element found in config file: " +
			config_file);
	}

	// Extract the robotModelFile
	tinyxml2::XMLElement* robotModelFile =
		root->FirstChildElement("robotModelFile");
	if (!robotModelFile) {
		throw std::runtime_error(
			"No 'robotModelFile' element found in config file: " + config_file);
	}
	config.robot_model_file = robotModelFile->GetText();

	// robot name
	tinyxml2::XMLElement* robotName = root->FirstChildElement("robotName");
	if (!robotName) {
		throw std::runtime_error(
			"No 'robotName' element found in config file: " + config_file);
	}
	config.robot_name = robotName->GetText();

	// robot base in world
	tinyxml2::XMLElement* robotBaseInWorld =
		root->FirstChildElement("robotBaseInWorld");
	if (robotBaseInWorld) {
		config.robot_base_in_world = parsePoseLocal(robotBaseInWorld);
	}

	// world gravity
	tinyxml2::XMLElement* worldGravity =
		root->FirstChildElement("worldGravity");
	if (worldGravity) {
		config.world_gravity = parseVector3dLocal(worldGravity);
	}

	// timestep
	tinyxml2::XMLElement* timestep = root->FirstChildElement("timestep");
	if (timestep) {
		config.timestep = timestep->DoubleText();
	}

	// extract logger config
	tinyxml2::XMLElement* logger = root->FirstChildElement("logger");
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
			 root->FirstChildElement("controller");
		 controller;
		 controller = controller->NextSiblingElement("controller")) {
		// get controller name
		const char* name = controller->Attribute("name");
		if (!name) {
			throw std::runtime_error(
				"controllers must have a name in config file: " + config_file);
		}
		if (name == std::string("")) {
			throw std::runtime_error(
				"controllers must have a non-empty name in config file: " +
				config_file);
		}
		if (config.controllers_configs.find(name) !=
			config.controllers_configs.end()) {
			throw std::runtime_error(
				"controllers must have a unique name in config file: " +
				config_file);
		}

		config.controllers_configs[name] =
			parseSingleControllerConfig(controller);

		if (config.initial_active_controller_name == "") {
			config.initial_active_controller_name = name;
		}
	}
	return config;
}

std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>>
RobotControllerConfigParser::parseSingleControllerConfig(
	tinyxml2::XMLElement* xml) {
	std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>> configs;
	// loop over tasks
	for (tinyxml2::XMLElement* task = xml->FirstChildElement("task"); task;
		 task = task->NextSiblingElement("task")) {
		// get task type
		const char* type = task->Attribute("type");
		if (!type) {
			throw std::runtime_error("tasks must have a type in config file: " +
									 _config_file_name);
		}
		if (std::string(type) == "joint_task") {
			configs.push_back(parseJointTaskConfig(task));
		} else if (std::string(type) == "motion_force_task") {
			configs.push_back(parseMotionForceTaskConfig(task));
		} else {
			throw std::runtime_error("Unknown task type: " + std::string(type));
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
		throw std::runtime_error("tasks must have a name in config file: " +
								 _config_file_name);
	}
	config.task_name = name;

	// get controlled joints
	tinyxml2::XMLElement* controlled_joints =
		xml->FirstChildElement("controlledJointNames");
	if (controlled_joints && controlled_joints->GetText()) {
		config.controlled_joint_names =
			splitString(controlled_joints->GetText(), ' ');
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
		JointTaskConfig::JointVelSatConfig vel_sat_config;
		// enabled
		vel_sat_config.enabled = velocity_saturation->BoolAttribute("enabled");

		const char* velocity_limits =
			velocity_saturation->Attribute("velocity_limit");
		if (velocity_limits) {
			std::vector<std::string> vectorVelocityLimits =
				splitString(velocity_limits, ' ');
			Eigen::VectorXd vel_limits_vec =
				Eigen::VectorXd(vectorVelocityLimits.size());
			for (int i = 0; i < vectorVelocityLimits.size(); i++) {
				vel_limits_vec[i] = std::stod(vectorVelocityLimits[i]);
			}
			vel_sat_config.velocity_limits = vel_limits_vec;
		} else if (vel_sat_config.enabled) {
			throw std::runtime_error(
				"velocitySaturation must have a velocity_limit if enabled "
				"in "
				"config file: " +
				_config_file_name);
		}
		config.velocity_saturation_config = vel_sat_config;
	}

	// otg
	tinyxml2::XMLElement* otg = xml->FirstChildElement("otg");
	if (otg) {
		config.otg_config = parseOTGJointConfig(otg, _config_file_name);
	}

	// gains
	tinyxml2::XMLElement* gains = xml->FirstChildElement("gains");
	if (gains) {
		config.gains_config = parseGainsConfig(gains, _config_file_name,
											   "joint task gains", false);
	}

	return config;
}

MotionForceTaskConfig RobotControllerConfigParser::parseMotionForceTaskConfig(
	tinyxml2::XMLElement* xml) {
	MotionForceTaskConfig config;
	// get name
	const char* name = xml->Attribute("name");
	if (!name) {
		throw std::runtime_error("tasks must have a name in config file: " +
								 _config_file_name);
	}
	config.task_name = name;

	// link name
	const char* link_name = xml->FirstChildElement("linkName")->GetText();
	if (!link_name) {
		throw std::runtime_error("tasks must have a linkName in config file: " +
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
		std::vector<Eigen::Vector3d> controlled_directions;
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
		std::vector<Eigen::Vector3d> controlled_directions;
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
	tinyxml2::XMLElement* force_motion_space =
		xml->FirstChildElement("forceSpaceParametrization");
	if (force_motion_space) {
		MotionForceTaskConfig::ForceMotionSpaceParamConfig
			force_motion_space_config;
		force_motion_space_config.force_space_dimension =
			force_motion_space->IntAttribute("forceSpaceDimension");
		force_motion_space_config.axis = parseVector3dLocal(force_motion_space);
		config.force_space_param_config = force_motion_space_config;
	}

	// moment space param
	tinyxml2::XMLElement* moment_space_param =
		xml->FirstChildElement("momentSpaceParametrization");
	if (moment_space_param) {
		MotionForceTaskConfig::ForceMotionSpaceParamConfig
			moment_space_param_config;
		moment_space_param_config.force_space_dimension =
			moment_space_param->IntAttribute("momentSpaceDimension");
		moment_space_param_config.axis = parseVector3dLocal(moment_space_param);
		config.moment_space_param_config = moment_space_param_config;
	}

	// velocity saturation
	tinyxml2::XMLElement* velocity_saturation =
		xml->FirstChildElement("velocitySaturation");
	if (velocity_saturation) {
		MotionForceTaskConfig::VelSatConfig vel_sat_config;
		// enabled
		vel_sat_config.enabled = velocity_saturation->BoolAttribute("enabled");

		const char* linear_velocity_limits =
			velocity_saturation->Attribute("linear_velocity_limit");
		if (linear_velocity_limits) {
			vel_sat_config.linear_velocity_limits =
				std::stod(linear_velocity_limits);
		} else if (vel_sat_config.enabled) {
			throw std::runtime_error(
				"velocitySaturation must have a linear_velocity_limit if "
				"enabled in config file: " +
				_config_file_name);
		}

		const char* angular_velocity_limits =
			velocity_saturation->Attribute("angular_velocity_limit");
		if (angular_velocity_limits) {
			vel_sat_config.angular_velocity_limits =
				std::stod(angular_velocity_limits);
		} else if (vel_sat_config.enabled) {
			throw std::runtime_error(
				"velocitySaturation must have a angular_velocity_limit if "
				"enabled in config file: " +
				_config_file_name);
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
			throw std::runtime_error("otg must have a type in config file: " +
									 _config_file_name);
		}
		if (std::string(type) == "disabled") {
			otg_config.enabled = false;
		} else if (std::string(type) == "acceleration") {
			otg_config.enabled = true;
			otg_config.jerk_limited = false;
		} else if (std::string(type) == "jerk") {
			otg_config.enabled = true;
			otg_config.jerk_limited = true;
		} else {
			throw std::runtime_error("Unknown otg type: " + std::string(type));
		}

		// velocity limits
		const char* linear_velocity_limits =
			otg->Attribute("max_linear_velocity");
		if (linear_velocity_limits) {
			otg_config.linear_velocity_limit =
				std::stod(linear_velocity_limits);
		} else if (otg_config.enabled) {
			throw std::runtime_error(
				"otg must have a max_linear_velocity if not disabled in config "
				"file: " +
				_config_file_name);
		}

		const char* angular_velocity_limits =
			otg->Attribute("max_angular_velocity");
		if (angular_velocity_limits) {
			otg_config.angular_velocity_limit =
				std::stod(angular_velocity_limits);
		} else if (otg_config.enabled) {
			throw std::runtime_error(
				"otg must have a max_angular_velocity if not disabled in "
				"config file: " +
				_config_file_name);
		}

		// acceleration limit
		const char* linear_acceleration_limit =
			otg->Attribute("max_linear_acceleration");
		if (linear_acceleration_limit) {
			otg_config.linear_acceleration_limit =
				std::stod(linear_acceleration_limit);
		} else if (otg_config.enabled) {
			throw std::runtime_error(
				"otg must have a max_linear_acceleration if not disabled in "
				"config file: " +
				_config_file_name);
		}

		const char* angular_acceleration_limit =
			otg->Attribute("max_angular_acceleration");
		if (angular_acceleration_limit) {
			otg_config.angular_acceleration_limit =
				std::stod(angular_acceleration_limit);
		} else if (otg_config.enabled) {
			throw std::runtime_error(
				"otg must have a max_angular_acceleration if not disabled in "
				"config file: " +
				_config_file_name);
		}

		// jerk limit
		const char* linear_jerk_limit = otg->Attribute("max_linear_jerk");
		if (linear_jerk_limit) {
			otg_config.linear_jerk_limit = std::stod(linear_jerk_limit);
		} else if (otg_config.enabled && otg_config.jerk_limited) {
			throw std::runtime_error(
				"otg must have a max_linear_jerk if jerk limited in config "
				"file: " +
				_config_file_name);
		}

		const char* angular_jerk_limit = otg->Attribute("max_angular_jerk");
		if (angular_jerk_limit) {
			otg_config.angular_jerk_limit = std::stod(angular_jerk_limit);
		} else if (otg_config.enabled && otg_config.jerk_limited) {
			throw std::runtime_error(
				"otg must have a max_angular_jerk if jerk limited in config "
				"file: " +
				_config_file_name);
		}
		config.otg_config = otg_config;
	}

	// position gains
	tinyxml2::XMLElement* position_gains =
		xml->FirstChildElement("positionGains");
	if (position_gains) {
		config.position_gains_config = parseGainsConfig(
			position_gains, _config_file_name, "positionGains", true);
	}

	// orientation gains
	tinyxml2::XMLElement* orientation_gains =
		xml->FirstChildElement("orientationGains");
	if (orientation_gains) {
		config.orientation_gains_config = parseGainsConfig(
			orientation_gains, _config_file_name, "orientationGains", true);
	}

	// force gains
	tinyxml2::XMLElement* force_gains = xml->FirstChildElement("forceGains");
	if (force_gains) {
		config.force_gains_config = parseGainsConfig(
			force_gains, _config_file_name, "forceGains", true);
	}

	// moment gains
	tinyxml2::XMLElement* moment_gains = xml->FirstChildElement("momentGains");
	if (moment_gains) {
		config.moment_gains_config = parseGainsConfig(
			moment_gains, _config_file_name, "momentGains", true);
	}

	return config;
}

}  // namespace Sai2Interfaces