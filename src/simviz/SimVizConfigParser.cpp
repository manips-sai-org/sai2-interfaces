#include "SimVizConfigParser.h"

#include <tinyxml2.h>
// #include <urdf/urdfdom_headers/urdf_model/include/urdf_model/pose.h>
// #include <urdf/urdfdom_headers/urdf_model/include/urdf_model/pose.h>
#include <iostream>
#include <urdf/urdfdom/urdf_parser/src/pose.cpp>

namespace {

Eigen::Affine3d parsePoseLocal(tinyxml2::XMLElement* xml) {
	Eigen::Affine3d pose = Eigen::Affine3d::Identity();

	urdf::Pose pose_urdf;
	urdf::parsePose(pose_urdf, xml);

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

	tinyxml2::XMLElement* root = doc.FirstChildElement("configuration");
	if (!root) {
		throw std::runtime_error(
			"No 'configuration' element found in config file: " + config_file);
	}

	// Extract the worldFilePath
	const char* worldFilePath =
		root->FirstChildElement("worldFilePath")->GetText();
	if (!worldFilePath) {
		throw std::runtime_error(
			"No 'worldFilePath' element found in config file: " + config_file);
	}
	config.world_file = worldFilePath;

	// Extract simParameters
	tinyxml2::XMLElement* simParams = root->FirstChildElement("simParameters");
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
				simParams->FirstChildElement("coeffFriction")
					->DoubleText();
		}

		if (simParams->FirstChildElement("collisionRestitution")) {
			config.collision_restitution =
				simParams->FirstChildElement("collisionRestitution")
					->DoubleText();
		}
	}

	// Extract forceSensor elements
	for (tinyxml2::XMLElement* forceSensor =
			 root->FirstChildElement("forceSensor");
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

	return config;
}

}  // namespace Sai2Interfaces