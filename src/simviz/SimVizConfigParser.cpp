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

	// Extract the simvizConfiguration element
	tinyxml2::XMLElement* simviz_config_xml =
		doc.FirstChildElement("simvizConfiguration");
	if (!simviz_config_xml) {
		throw std::runtime_error(
			"No 'simvizConfiguration' element found in config file: " +
			config_file);
	}

	// error out if there are multiple simvizConfiguration elements
	if (simviz_config_xml->NextSiblingElement("simvizConfiguration")) {
		throw std::runtime_error(
			"Only one 'simvizConfiguration' element is allowed in config "
			"file: " +
			config_file);
	}

	// Extract the worldFilePath
	// tinyxml2::XMLElement* worldFilePath =
	// simviz_config_xml->FirstChildElement("worldFilePath");
	if (!simviz_config_xml->Attribute("worldFilePath")) {
		throw std::runtime_error(
			"No 'worldFilePath' Attribute found in simVizConfiguration element "
			"in config file: " +
			config_file);
	}
	config.world_file = simviz_config_xml->Attribute("worldFilePath");

	// get the redis prefix
	if (simviz_config_xml->Attribute("redisPrefix")) {
		config.redis_prefix = simviz_config_xml->Attribute("redisPrefix");
	}

	// Extract the simviz mode
	if (simviz_config_xml->Attribute("mode")) {
		std::string mode_str = simviz_config_xml->Attribute("mode");
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
		if (simParams->NextSiblingElement("simParameters")) {
			throw std::runtime_error(
				"Only one 'simParameters' element is allowed per "
				"'simvizConfiguration'element in config file: " +
				config_file);
		}
		if (simParams->Attribute("timestep")) {
			config.timestep = simParams->DoubleAttribute("timestep");
		}
		if (simParams->Attribute("speedupFactor")) {
			config.speedup_factor = simParams->DoubleAttribute("speedupFactor");
		}
		if (simParams->Attribute("enableJointLimits")) {
			config.enable_joint_limits =
				simParams->BoolAttribute("enableJointLimits");
		}
		if (simParams->Attribute("frictionCoefficient")) {
			config.global_friction_coefficient =
				simParams->DoubleAttribute("frictionCoefficient");
		}
		if (simParams->Attribute("collisionRestitutionCoefficient")) {
			config.global_collision_restitution =
				simParams->DoubleAttribute("collisionRestitutionCoefficient");
		}
		if (simParams->Attribute("enableGravityCompensation")) {
			config.enable_gravity_compensation =
				simParams->BoolAttribute("enableGravityCompensation");
		}
	}

	// Extract model specific dynamic and rendering parameters
	for (tinyxml2::XMLElement* modelParams =
			 simviz_config_xml->FirstChildElement(
				 "robotOrObjectSpecificParameters");
		 modelParams; modelParams = modelParams->NextSiblingElement(
						  "robotOrObjectSpecificParameters")) {
		DynamicAndRenderingParams params;

		if (!modelParams->Attribute("name")) {
			throw std::runtime_error(
				"Robot or object specific parameters must have a name "
				"attribute");
		}
		std::string name = modelParams->Attribute("name");

		if (modelParams->Attribute("dynamicsEnabled")) {
			params.dynamics_enabled =
				modelParams->BoolAttribute("dynamicsEnabled");
		}
		if (modelParams->Attribute("renderingEnabled")) {
			params.rendering_enabled =
				modelParams->BoolAttribute("renderingEnabled");
		}
		if (modelParams->Attribute("jointLimitsEnabled")) {
			params.joint_limits_enabled =
				modelParams->BoolAttribute("jointLimitsEnabled");
		}
		if (modelParams->Attribute("collisionRestitutionCoefficient")) {
			params.collision_restitution_coefficient =
				modelParams->DoubleAttribute("collisionRestitutionCoefficient");
		}
		if (modelParams->Attribute("frictionCoefficient")) {
			params.friction_coefficient =
				modelParams->DoubleAttribute("frictionCoefficient");
		}
		if (modelParams->Attribute("wireMeshRenderingMode")) {
			params.wire_mesh_rendering_mode =
				modelParams->BoolAttribute("wireMeshRenderingMode");
		}
		if (modelParams->Attribute("framesRenderingEnabled")) {
			params.frames_rendering_enabled =
				modelParams->BoolAttribute("framesRenderingEnabled");
		}
		if (modelParams->Attribute("frameSizeWhenRendering")) {
			params.frames_size_when_rendering =
				modelParams->DoubleAttribute("frameSizeWhenRendering");
		}
		config.model_specific_dynamic_and_rendering_params[name] = params;
	}

	// Extract forceSensor elements
	for (tinyxml2::XMLElement* forceSensor =
			 simviz_config_xml->FirstChildElement("forceSensor");
		 forceSensor;
		 forceSensor = forceSensor->NextSiblingElement("forceSensor")) {
		SimForceSensorConfig force_sensor_config;

		if (!forceSensor->Attribute("robotOrObjectName")) {
			throw std::runtime_error(
				"Force sensor must have a 'robotOrObjectName' attribute");
		}
		force_sensor_config.robot_or_object_name =
			forceSensor->Attribute("robotOrObjectName");
		if (forceSensor->Attribute("linkName")) {
			force_sensor_config.link_name = forceSensor->Attribute("linkName");
		}
		if (forceSensor->Attribute("filterCutoff")) {
			force_sensor_config.cutoff_frequency =
				forceSensor->DoubleAttribute("filterCutoff");
		}
		tinyxml2::XMLElement* origin = forceSensor->FirstChildElement("origin");
		if (origin) {
			force_sensor_config.transform_in_link = parsePoseLocal(origin);
		}
		config.force_sensors.push_back(force_sensor_config);
	}

	// extract logger config
	tinyxml2::XMLElement* logger =
		simviz_config_xml->FirstChildElement("logger");
	if (logger) {
		if (logger->NextSiblingElement("logger")) {
			throw std::runtime_error(
				"Only one 'logger' element is allowed per "
				"'simvizConfiguration' "
				"element in config file: " +
				config_file);
		}

		if (logger->Attribute("logFolderName")) {
			config.logger_config.folder_name =
				logger->Attribute("logFolderName");
		}

		if (logger->Attribute("logFrequency")) {
			config.logger_config.frequency =
				logger->DoubleAttribute("logFrequency");
		}

		if (logger->Attribute("startWithSimulation")) {
			config.logger_config.start_with_logger_on =
				logger->BoolAttribute("startWithSimulation");
		}

		if (logger->Attribute("timestampInFilename")) {
			config.logger_config.add_timestamp_to_filename =
				logger->BoolAttribute("timestampInFilename");
		}
	}

	return config;
}

}  // namespace Sai2Interfaces