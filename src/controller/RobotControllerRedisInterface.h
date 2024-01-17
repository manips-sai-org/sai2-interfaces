#ifndef SAI2_INTERFACES_ROBOT_CONTROLLER_REDIS_INTERFACE_H
#define SAI2_INTERFACES_ROBOT_CONTROLLER_REDIS_INTERFACE_H

#include <string>

#include "RobotControllerConfigParser.h"
#include "Sai2Model.h"
#include "Sai2Primitives.h"
#include "redis/RedisClient.h"

namespace Sai2Interfaces {

class RobotControllerRedisInterface {
public:
	RobotControllerRedisInterface(const std::string& config_file);
	~RobotControllerRedisInterface() = default;

	void run();

private:
	struct JointTaskInput {
		Eigen::VectorXd goal_position;
		Eigen::VectorXd goal_velocity;
		Eigen::VectorXd goal_acceleration;

		JointTaskInput(){
			goal_position.setZero(1);
			goal_velocity.setZero(1);
			goal_acceleration.setZero(1);
		
		};
		JointTaskInput(const int& dofs) {
			goal_position.setZero(dofs);
			goal_velocity.setZero(dofs);
			goal_acceleration.setZero(dofs);
		}
	};

	struct MotionForceTaskInput {
		Eigen::Vector3d goal_position;
		Eigen::Vector3d goal_linear_velocity;
		Eigen::Vector3d goal_linear_acceleration;
		Eigen::Matrix3d goal_orientation;
		Eigen::Vector3d goal_angular_velocity;
		Eigen::Vector3d goal_angular_acceleration;
		Eigen::Vector3d desired_force;
		Eigen::Vector3d desired_moment;
		Eigen::Vector3d sensed_force;
		Eigen::Vector3d sensed_moment;

		MotionForceTaskInput() {
			goal_position.setZero();
			goal_linear_velocity.setZero();
			goal_linear_acceleration.setZero();
			goal_orientation.setIdentity();
			goal_angular_velocity.setZero();
			goal_angular_acceleration.setZero();
			desired_force.setZero();
			desired_moment.setZero();
			sensed_force.setZero();
			sensed_moment.setZero();
		}
	};

	typedef std::variant<JointTaskInput, MotionForceTaskInput> TaskInputVariant;

	void initialize();

	void completeConfigFromTaskDefaultValues();
	void initializeRedisTasksIO();

	void switchController(const std::string& controller_name);

	void processInputs();

	std::string _config_file;
	RobotControllerConfigParser _config_parser;
	RobotControllerConfig _config;

	std::shared_ptr<Sai2Model::Sai2Model> _robot_model;
	std::map<std::string, std::unique_ptr<Sai2Primitives::RobotController>>
		_robot_controllers;
	std::string _active_controller_name;

	Sai2Common::RedisClient _redis_client;

	Eigen::VectorXd _robot_q;
	Eigen::VectorXd _robot_dq;
	Eigen::VectorXd _robot_command_torques;

	std::map<std::string, std::map<std::string, TaskInputVariant>>
		_controller_inputs;
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_ROBOT_CONTROLLER_REDIS_INTERFACE_H