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

/**
 * @brief A config object for the gains of a controller. It contains the P, D,
 * and I gains, as well as a flag to enable gains safety checks (such as making
 * sure that the gains are positive).
 *
 * In the xml config file, several elements will be parsed to this struct:
 * - In JointTaskConfig:
 * 		- \b gains_config from: <gains kp="..." kv="..." ki="..." />
 * - In MotionForceTaskConfig:
 * 		- \b position_gains_config from: <positionGains kp="..." kv="..."
 * ki="..." />
 * 		- \b orientation_gains_config from: <orientationGains kp="..." kv="..."
 * ki="..." />
 * 		- \b force_gains_config from: <forceGains kp="..." kv="..." ki="..." />
 * 		- \b moment_gains_config from: <momentGains kp="..." kv="..." ki="..."
 * />
 *
 * The gains are stored in Eigen::VectorXd objects, which allows for the gains
 * to be either isotropic (same gain in every direction) when the size of the
 * vector is 1, or anisotropic (different gains in different directions) when
 * the size of the vector is greater than 1. If the gains are inosotropic, the
 * size of the vector should correspond to the number of degrees of freedom of
 * the controlled task (i.e. 3 for a position task, n for a joint task on a
 * robot with n degrees of freedom, ...).
 *
 */
struct GainsConfig {
	bool safety_checks_enabled = true;	///< Flag to enable gains safety checks
	Eigen::VectorXd kp;					///< Proportional gains
	Eigen::VectorXd kv;					///< Derivative gains
	Eigen::VectorXd ki;					///< Integral gains

	/**
	 * @brief Construct a new GainsConfig object with default gains of 0, and
	 * gains vector of size 1.
	 *
	 */
	GainsConfig() {
		kp.setZero(1);
		kv.setZero(1);
		ki.setZero(1);
	}

	/**
	 * @brief Construct a new Gains Config object with vectors of size 1 and
	 * gains values provided as arguments.
	 *
	 * @param kp
	 * @param kv
	 * @param ki
	 */
	GainsConfig(const double kp, const double kv, const double ki) {
		this->kp = Eigen::VectorXd::Constant(1, kp);
		this->kv = Eigen::VectorXd::Constant(1, kv);
		this->ki = Eigen::VectorXd::Constant(1, ki);
	}
};

/**
 * @brief Config for the logger attached to the RobotControllerRedisInterface.
 * 
 * This is parsed from the xml config file, from the following element:
 * 
 * 
 */
struct ControllerLoggerConfig {
	std::string folder_name = "log_files/controllers";
	double frequency = 100.0;
	bool start_with_logger_on = false;
	bool add_timestamp_to_filename = true;
};

struct MotionForceTaskInterfaceConfig {
	std::string min_goal_position = "[-0.5,-0.5,0.0]";
	std::string max_goal_position = "[0.5,0.5,0.8]";
	std::string min_desired_force = "-50";
	std::string max_desired_force = "50";
	std::string min_desired_moment = "-5";
	std::string max_desired_moment = "5";
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
				JointTaskDefaultParams::otg_max_jerk * Eigen::VectorXd::Ones(1);
		};

		bool enabled = JointTaskDefaultParams::use_internal_otg;
		bool jerk_limited = JointTaskDefaultParams::internal_otg_jerk_limited;
		JointOTGLimit limits;
	};

	std::string task_name = "";
	std::vector<std::string> controlled_joint_names = {};
	bool use_dynamic_decoupling =
		JointTaskDefaultParams::dynamic_decoupling_type !=
		Sai2Primitives::DynamicDecouplingType::IMPEDANCE;
	double bie_threshold = JointTaskDefaultParams::bie_threshold;

	JointVelSatConfig velocity_saturation_config;
	JointOTGConfig otg_config;
	GainsConfig gains_config =
		GainsConfig(JointTaskDefaultParams::kp, JointTaskDefaultParams::kv,
					JointTaskDefaultParams::ki);
};

struct MotionForceTaskConfig {
	struct ForceMotionSpaceParamConfig {
		int force_space_dimension;
		Eigen::Vector3d axis;

		ForceMotionSpaceParamConfig()
			: force_space_dimension(0), axis(Eigen::Vector3d::UnitZ()) {}

		ForceMotionSpaceParamConfig(
			const int force_space_dimension,
			const Eigen::Vector3d& axis = Eigen::Vector3d::UnitZ())
			: force_space_dimension(force_space_dimension), axis(axis) {}
	};

	struct ForceControlConfig {
		bool closed_loop_force_control = false;
		Eigen::Affine3d force_sensor_frame = Eigen::Affine3d::Identity();
		ForceMotionSpaceParamConfig force_space_param_config =
			ForceMotionSpaceParamConfig(
				MotionForceTaskDefaultParams::force_space_dimension);
		ForceMotionSpaceParamConfig moment_space_param_config =
			ForceMotionSpaceParamConfig(
				MotionForceTaskDefaultParams::moment_space_dimension);
		GainsConfig force_gains_config =
			GainsConfig(MotionForceTaskDefaultParams::kp_force,
						MotionForceTaskDefaultParams::kv_force,
						MotionForceTaskDefaultParams::ki_force);
		GainsConfig moment_gains_config =
			GainsConfig(MotionForceTaskDefaultParams::kp_moment,
						MotionForceTaskDefaultParams::kv_moment,
						MotionForceTaskDefaultParams::ki_moment);
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
		Sai2Primitives::DynamicDecouplingType::IMPEDANCE;
	double bie_threshold = MotionForceTaskDefaultParams::bie_threshold;

	std::vector<Eigen::Vector3d> controlled_directions_position = {
		Vector3d::UnitX(), Vector3d::UnitY(), Vector3d::UnitZ()};
	std::vector<Eigen::Vector3d> controlled_directions_orientation = {
		Vector3d::UnitX(), Vector3d::UnitY(), Vector3d::UnitZ()};

	ForceControlConfig force_control_config;

	VelSatConfig velocity_saturation_config;
	OTGConfig otg_config;
	GainsConfig position_gains_config =
		GainsConfig(MotionForceTaskDefaultParams::kp_pos,
					MotionForceTaskDefaultParams::kv_pos,
					MotionForceTaskDefaultParams::ki_pos);
	GainsConfig orientation_gains_config =
		GainsConfig(MotionForceTaskDefaultParams::kp_ori,
					MotionForceTaskDefaultParams::kv_ori,
					MotionForceTaskDefaultParams::ki_ori);

	MotionForceTaskInterfaceConfig interface_config;
};

struct RobotControllerConfig {
	std::string robot_model_file = "";
	std::string robot_name = "";
	std::string redis_prefix = "sai2::interfaces";
	Eigen::Affine3d robot_base_in_world = Eigen::Affine3d::Identity();
	Eigen::Vector3d world_gravity = Eigen::Vector3d(0, 0, -9.81);
	double control_frequency = 1000.0;

	ControllerLoggerConfig logger_config;

	std::string initial_active_controller_name = "";

	std::map<std::string,
			 std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>>>
		controllers_configs = {};
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_H
