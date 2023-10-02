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
		// if (!simParams->QueryDoubleAttribute("timestep", &config.timestep) ==
		// 	tinyxml2::XML_SUCCESS) {
		// 	std::cout << "Timestep not found in config file." << std::endl;
		// }

		// tinyxml2::XMLElement* timestepElement =
		// simParams->FirstChildElement("timestep");
		if (simParams->FirstChildElement("timestep")) {
			config.timestep =
				simParams->FirstChildElement("timestep")->DoubleText();
		}

		if (simParams->FirstChildElement("enableJointLimits")) {
			config.enable_joint_limits =
				simParams->FirstChildElement("enableJointLimits")->BoolText();
		}

		if (simParams->FirstChildElement("frictionCoefficient")) {
			config.friction_coefficient =
				simParams->FirstChildElement("frictionCoefficient")
					->DoubleText();
		}

		if (simParams->FirstChildElement("collisionRestitution")) {
			config.collision_restitution =
				simParams->FirstChildElement("collisionRestitution")
					->DoubleText();
		}

		// tinyxml2::XMLElement* enable_join =
		// simParams->FirstChildElement("timestep");

		//     // Get the text value and convert it to a double
		//     const char* timestepValue = timestepElement->GetText();
		//     if (timestepValue) {
		//         double timestep;
		//         if (tinyxml2::XML_SUCCESS ==
		//         timestepElement->QueryDoubleText(&timestep)) {
		//             std::cout << "Timestep: " << timestep << std::endl;
		//         } else {
		//             std::cerr << "Invalid 'timestep' element text value." <<
		//             std::endl;
		//         }
		//     } else {
		//         std::cerr << "No text value found for 'timestep' element." <<
		//         std::endl;
		//     }
		// } else {
		//     std::cerr << "'timestep' element not found in 'simParameters'."
		//     << std::endl;
		// }

		// config.timestep =
		// simParams->FirstChildElement("timestep")->DoubleText();
		// config.enable_joint_limits =
		// simParams->FirstChildElement("enableJointLimits")->BoolText();
		// config.friction_coefficient =
		// simParams->FirstChildElement("frictionCoefficient")->DoubleText();
		// config.collision_restitution =
		// simParams->FirstChildElement("collisionRestitution")->DoubleText();

		// // if (simParams) {
		//         const char* timestepAttr = simParams->Attribute("timestep");
		//         if (timestepAttr) {
		//             double timestep;
		//             if (tinyxml2::XML_SUCCESS ==
		//             simParams->QueryDoubleAttribute("timestep", &timestep)) {
		//                 std::cout << "Timestep: " << timestep << std::endl;
		//             } else {
		//                 std::cerr << "Invalid 'timestep' attribute value." <<
		//                 std::endl;
		//             }
		//         } else {
		//             std::cerr << "'timestep' attribute not found in
		//             'simParameters'." << std::endl;
		//         }
		//     } else {
		//         std::cerr << "'simParameters' element not found." <<
		//         std::endl;
		// // }

		// if (!simParams->QueryBoolAttribute("enableJointLimits",
		// 								   &config.enable_joint_limits) ==
		// 	tinyxml2::XML_SUCCESS) {
		// 	std::cout << "Enable Joint Limits not found in config file."
		// 			  << std::endl;
		// }

		// if (!simParams->QueryDoubleAttribute("frictionCoefficient",
		// 									 &config.friction_coefficient) ==
		// 	tinyxml2::XML_SUCCESS) {
		// 	std::cout << "Friction Coefficient not found in config file."
		// 			  << std::endl;
		// }

		// if (!simParams->QueryDoubleAttribute("collisionRestitution",
		// 									 &config.collision_restitution) ==
		// 	tinyxml2::XML_SUCCESS) {
		// 	std::cout << "Collision Restitution not found in config file."
		// 			  << std::endl;
		// }
	}

	// Extract forceSensor elements (assuming multiple)
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

		// force_sensor_config.robot_name =
		// 	forceSensor->FirstChildElement("robotName")->GetText();
		// force_sensor_config.link_name =
		// 	forceSensor->FirstChildElement("linkName")->GetText();

		tinyxml2::XMLElement* origin = forceSensor->FirstChildElement("origin");
		if (origin) {
			force_sensor_config.transform_in_link = parsePoseLocal(origin);
		}

		if (forceSensor->FirstChildElement("filterCutoff")) {
			force_sensor_config.cutoff_frequency =
				forceSensor->FirstChildElement("linkName")->DoubleText();
		}

		// forceSensor->QueryDoubleAttribute("filterCutoff",
		// 								&force_sensor_config.cutoff_frequency);
		config.force_sensors.push_back(force_sensor_config);
	}

	return config;
}

}  // namespace Sai2Interfaces