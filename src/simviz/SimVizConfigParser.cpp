#include "SimVizConfigParser.h"

#include <tinyxml2.h>
#include <urdf/urdfdom_headers/urdf_model/include/urdf_model/pose.h>

#include <iostream>

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

Eigen::Affine3d parsePoseLocal(tinyxml2::XMLElement* xml) {
	Eigen::Affine3d pose = Eigen::Affine3d::Identity();

	Sai2Urdfreader::Pose pose_urdf;
	parsePose(pose_urdf, xml);

	pose.translation() << pose_urdf.position.x, pose_urdf.position.y,
		pose_urdf.position.z;
	pose.linear() =
		Eigen::Quaterniond(pose_urdf.rotation.w, pose_urdf.rotation.x,
						   pose_urdf.rotation.y, pose_urdf.rotation.z)
			.toRotationMatrix();

	return pose;
}

}  // namespace

namespace Sai2Interfaces {

SimVizConfig SimVizConfigParser::parseConfig(const std::string& config_file) {
	SimVizConfig config;

	tinyxml2::XMLDocument doc;
	if (doc.LoadFile(config_file.c_str()) != tinyxml2::XML_SUCCESS) {
		throw std::runtime_error("Could not load simviz config file: " +
								 config_file);
	}

	tinyxml2::XMLElement* simviz_config_xml =
		doc.FirstChildElement("simvizConfiguration");
	if (!simviz_config_xml) {
		throw std::runtime_error(
			"No 'simvizConfiguration' element found in config file: " +
			config_file);
	}

	// read the config from the referenced file if it exists
	tinyxml2::XMLElement* simviz_config_xml_2;
	if (simviz_config_xml->Attribute("file")) {
		const std::string internal_config_file =
			simviz_config_xml->Attribute("file");
		return parseConfig(internal_config_file);
	}

	// Extract the worldFilePath
	tinyxml2::XMLElement* worldFilePath =
		simviz_config_xml->FirstChildElement("worldFilePath");
	if (!worldFilePath) {
		throw std::runtime_error(
			"No 'worldFilePath' element found in config file: " + config_file);
	}
	config.world_file = worldFilePath->GetText();

	// Extract the simviz mode
	tinyxml2::XMLElement* mode = simviz_config_xml->FirstChildElement("mode");
	if (mode) {
		std::string mode_str = mode->GetText();
		if (mode_str == "simviz") {
			config.mode = SimVizMode::SIMVIZ;
		} else if (mode_str == "simOnly") {
			config.mode = SimVizMode::SIM_ONLY;
		} else if (mode_str == "vizOnly") {
			config.mode = SimVizMode::VIZ_ONLY;
		} else {
			throw std::runtime_error(
				"Invalid simviz mode: " + mode_str + " in config file: " +
				config_file + ". Valid modes are: simviz, simOnly, vizOnly");
		}
	}

	// Extract simParameters
	tinyxml2::XMLElement* simParams =
		simviz_config_xml->FirstChildElement("simParameters");
	if (simParams) {
		if (simParams->FirstChildElement("timestep")) {
			config.timestep =
				simParams->FirstChildElement("timestep")->DoubleText();
		}

		if (simParams->FirstChildElement("enableJointLimits")) {
			config.enable_joint_limits =
				simParams->FirstChildElement("enableJointLimits")->BoolText();
		}

		if (simParams->FirstChildElement("coeffFriction")) {
			config.friction_coefficient =
				simParams->FirstChildElement("coeffFriction")->DoubleText();
		}

		if (simParams->FirstChildElement("collisionRestitution")) {
			config.collision_restitution =
				simParams->FirstChildElement("collisionRestitution")
					->DoubleText();
		}
	}

	// Extract forceSensor elements
	for (tinyxml2::XMLElement* forceSensor =
			 simviz_config_xml->FirstChildElement("forceSensor");
		 forceSensor;
		 forceSensor = forceSensor->NextSiblingElement("forceSensor")) {
		SimForceSensorConfig force_sensor_config;

		if (forceSensor->FirstChildElement("robotName")) {
			force_sensor_config.robot_name =
				forceSensor->FirstChildElement("robotName")->GetText();
		}

		if (forceSensor->FirstChildElement("linkName")) {
			force_sensor_config.link_name =
				forceSensor->FirstChildElement("linkName")->GetText();
		}

		tinyxml2::XMLElement* origin = forceSensor->FirstChildElement("origin");
		if (origin) {
			force_sensor_config.transform_in_link = parsePoseLocal(origin);
		}

		if (forceSensor->FirstChildElement("filterCutoff")) {
			force_sensor_config.cutoff_frequency =
				forceSensor->FirstChildElement("filterCutoff")->DoubleText();
		}

		config.force_sensors.push_back(force_sensor_config);
	}

	// extract logger config
	tinyxml2::XMLElement* logger =
		simviz_config_xml->FirstChildElement("logger");
	if (logger) {
		if (logger->FirstChildElement("logFolderName")) {
			config.logger_config.folder_name =
				logger->FirstChildElement("logFolderName")->GetText();
		}

		if (logger->FirstChildElement("logFrequency")) {
			config.logger_config.frequency =
				logger->FirstChildElement("logFrequency")->DoubleText();
		}

		if (logger->FirstChildElement("startWithSimulation")) {
			config.logger_config.start_with_logger_on =
				logger->FirstChildElement("startWithSimulation")->BoolText();
		}

		if (logger->FirstChildElement("timestampInFilename")) {
			config.logger_config.add_timestamp_to_filename =
				logger->FirstChildElement("timestampInFilename")->BoolText();
		}
	}

	return config;
}

}  // namespace Sai2Interfaces