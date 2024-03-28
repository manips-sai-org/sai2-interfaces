#ifndef SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_H
#define SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_H

#include <Eigen/Dense>
#include <map>
#include <string>
#include <variant>

#include "Sai2Primitives.h"

using JointTaskDefaultParams = Sai2Primitives::JointTask::DefaultParameters;
using MotionForceTaskDefaultParams =
	Sai2Primitives::MotionForceTask::DefaultParameters;

namespace Sai2Interfaces {

struct GainsConfig {
	bool safety_checks_enabled = true;
	Eigen::VectorXd kp;
	Eigen::VectorXd kv;
	Eigen::VectorXd ki;

	GainsConfig() {
		kp.setZero(1);
		kv.setZero(1);
		ki.setZero(1);
	}

	GainsConfig(const double kp, const double kv, const double ki) {
		this->kp = Eigen::VectorXd::Constant(1, kp);
		this->kv = Eigen::VectorXd::Constant(1, kv);
		this->ki = Eigen::VectorXd::Constant(1, ki);
	}
};

struct ControllerLoggerConfig {
	std::string folder_name = "logs_control";
	double frequency = 100.0;
	bool start_with_logger_on = false;
	bool add_timestamp_to_filename = true;
};

struct JointTaskConfig {
	struct JointVelSatConfig {
		bool enabled = JointTaskDefaultParams::use_velocity_saturation;
		Eigen::VectorXd velocity_limits =
			JointTaskDefaultParams::saturation_velocity *
			Eigen::VectorXd::Ones(1);
	};

	struct JointOTGConfig {
		struct JointOTGLimit {
			Eigen::VectorXd velocity_limit =
				JointTaskDefaultParams::otg_max_velocity *
				Eigen::VectorXd::Ones(1);
			Eigen::VectorXd acceleration_limit =
				JointTaskDefaultParams::otg_max_acceleration *
				Eigen::VectorXd::Ones(1);
			Eigen::VectorXd jerk_limit =
				JointTaskDefaultParams::otg_max_jerk *
				Eigen::VectorXd::Ones(1);
		};

		bool enabled = JointTaskDefaultParams::use_internal_otg;
		bool jerk_limited = JointTaskDefaultParams::internal_otg_jerk_limited;
		JointOTGLimit limits;
	};

	std::string task_name = "";
	std::vector<std::string> controlled_joint_names = {};
	bool use_dynamic_decoupling =
		JointTaskDefaultParams::dynamic_decoupling_type !=
		Sai2Primitives::JointTask::DynamicDecouplingType::IMPEDANCE;

	std::optional<JointVelSatConfig> velocity_saturation_config = {};
	std::optional<JointOTGConfig> otg_config = {};
	std::optional<GainsConfig> gains_config = {};
};

struct MotionForceTaskConfig {
	struct ForceMotionSpaceParamConfig {
		int force_space_dimension =
			MotionForceTaskDefaultParams::force_space_dimension;
		Eigen::Vector3d axis = Eigen::Vector3d::UnitZ();
	};

	struct VelSatConfig {
		bool enabled = MotionForceTaskDefaultParams::use_velocity_saturation;
		double linear_velocity_limits =
			MotionForceTaskDefaultParams::linear_saturation_velocity;
		double angular_velocity_limits =
			MotionForceTaskDefaultParams::angular_saturation_velocity;
	};

	struct OTGConfig {
		bool enabled = MotionForceTaskDefaultParams::use_internal_otg;
		bool jerk_limited =
			MotionForceTaskDefaultParams::internal_otg_jerk_limited;
		double linear_velocity_limit =
			MotionForceTaskDefaultParams::otg_max_linear_velocity;
		double angular_velocity_limit =
			MotionForceTaskDefaultParams::otg_max_angular_velocity;
		double linear_acceleration_limit =
			MotionForceTaskDefaultParams::otg_max_linear_acceleration;
		double angular_acceleration_limit =
			MotionForceTaskDefaultParams::otg_max_angular_acceleration;
		double linear_jerk_limit =
			MotionForceTaskDefaultParams::otg_max_linear_jerk;
		double angular_jerk_limit =
			MotionForceTaskDefaultParams::otg_max_angular_jerk;
	};

	std::string task_name = "";
	std::string link_name = "";
	Eigen::Affine3d compliant_frame = Eigen::Affine3d::Identity();
	bool is_parametrization_in_compliant_frame = false;
	bool use_dynamic_decoupling =
		MotionForceTaskDefaultParams::dynamic_decoupling_type !=
		Sai2Primitives::MotionForceTask::DynamicDecouplingType::IMPEDANCE;

	std::optional<std::vector<Eigen::Vector3d>> controlled_directions_position =
		{};
	std::optional<std::vector<Eigen::Vector3d>>
		controlled_directions_orientation = {};

	bool closed_loop_force_control = false;
	Eigen::Affine3d force_sensor_frame = Eigen::Affine3d::Identity();
	std::optional<ForceMotionSpaceParamConfig> force_space_param_config = {};
	std::optional<ForceMotionSpaceParamConfig> moment_space_param_config = {};

	std::optional<VelSatConfig> velocity_saturation_config = {};
	std::optional<OTGConfig> otg_config = {};
	std::optional<GainsConfig> position_gains_config = {};
	std::optional<GainsConfig> orientation_gains_config = {};
	std::optional<GainsConfig> force_gains_config = {};
	std::optional<GainsConfig> moment_gains_config = {};
};

struct RobotControllerConfig {
	std::string robot_model_file = "";
	std::string robot_name = "";
	Eigen::Affine3d robot_base_in_world = Eigen::Affine3d::Identity();
	Eigen::Vector3d world_gravity = Eigen::Vector3d(0, 0, -9.81);
	double timestep = 0.001;

	ControllerLoggerConfig logger_config;

	std::string initial_active_controller_name = "";

	std::map<std::string,
			 std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>>>
		controllers_configs = {};
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_H
