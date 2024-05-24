#ifndef SAI2_INTERFACES_SIMVIZ_CONFIG_H
#define SAI2_INTERFACES_SIMVIZ_CONFIG_H

#include <Eigen/Dense>
#include <map>
#include <string>

namespace Sai2Interfaces {

enum SimVizMode { SIMVIZ = 0, SIM_ONLY = 1, VIZ_ONLY = 2 };

struct DynamicAndRenderingParams {
	bool dynamics_enabled = true;
	bool rendering_enabled = true;
	bool joint_limits_enabled = true;
	double collision_restitution_coefficient = 0.0;
	double static_friction_coefficient = 0.5;
	double dynamic_friction_coefficient = 0.5;
	bool wire_mesh_rendering_mode = false;
	bool frames_rendering_enabled = false;
	double frames_size_when_rendering = 0.2;

	bool operator==(const DynamicAndRenderingParams& other) const {
		return (dynamics_enabled == other.dynamics_enabled) &&
			   (rendering_enabled == other.rendering_enabled) &&
			   (joint_limits_enabled == other.joint_limits_enabled) &&
			   (collision_restitution_coefficient ==
				other.collision_restitution_coefficient) &&
			   (static_friction_coefficient ==
				other.static_friction_coefficient) &&
			   (dynamic_friction_coefficient ==
				other.dynamic_friction_coefficient) &&
			   (wire_mesh_rendering_mode == other.wire_mesh_rendering_mode) &&
			   (frames_rendering_enabled == other.frames_rendering_enabled) && 
			   (frames_size_when_rendering == other.frames_size_when_rendering);
	}
};

struct SimForceSensorConfig {
	std::string robot_name = "";
	std::string link_name = "";
	Eigen::Affine3d transform_in_link = Eigen::Affine3d::Identity();
	double cutoff_frequency = 0.0;

	bool operator==(const SimForceSensorConfig& other) const {
		return (robot_name == other.robot_name) &&
			   (link_name == other.link_name) &&
			   (transform_in_link.matrix() ==
				other.transform_in_link.matrix()) &&
			   (cutoff_frequency == other.cutoff_frequency);
	}
};

struct SimLoggerConfig {
	std::string folder_name = "log_files/simviz";
	double frequency = 100.0;
	bool start_with_logger_on = false;
	bool add_timestamp_to_filename = true;

	bool operator==(const SimLoggerConfig& other) const {
		return (folder_name == other.folder_name) &&
			   (frequency == other.frequency) &&
			   (start_with_logger_on == other.start_with_logger_on) &&
			   (add_timestamp_to_filename == other.add_timestamp_to_filename);
	}
};

struct SimVizConfig {
	std::string world_file = "";
	std::string redis_prefix = "sai2::interfaces";
	bool enable_joint_limits = true;
	bool enable_gravity_compensation = true;
	double global_friction_coefficient = 0.0;
	double global_collision_restitution = 0.0;
	double timestep = 0.001;
	double speedup_factor = 1.0;

	std::map<std::string, DynamicAndRenderingParams>
		model_specific_dynamic_and_rendering_params = {};

	SimVizMode mode = SimVizMode::SIMVIZ;

	std::vector<SimForceSensorConfig> force_sensors = {};

	SimLoggerConfig logger_config;

	bool operator==(const SimVizConfig& other) const {
		return (world_file == other.world_file) &&
			   (enable_joint_limits == other.enable_joint_limits) &&
			   (global_friction_coefficient ==
				other.global_friction_coefficient) &&
			   (global_collision_restitution ==
				other.global_collision_restitution) &&
			   (timestep == other.timestep) && (mode == other.mode) &&
			   (force_sensors == other.force_sensors) &&
			   (logger_config == other.logger_config);
	}
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_SIMVIZ_CONFIG_H
