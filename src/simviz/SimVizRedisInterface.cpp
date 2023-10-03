#include "SimVizRedisInterface.h"

#include <signal.h>

#include <thread>

namespace Sai2Interfaces {

namespace {
bool should_stop = false;
void stop(int i) { should_stop = true; }

// redis keys
const std::string group_name = "simviz_redis_group_name";

const std::string SIM_PAUSE_KEY = "sai2::interfaces::simviz::pause";
const std::string SIM_RESET_KEY = "sai2::interfaces::simviz::reset";
const std::string GRAV_COMP_ENABLED_KEY =
	"sai2::interfaces::simviz::gravity_comp_enabled";
const std::string CONFIG_FILE_KEY =
	"sai2::interfaces::simviz::config_file";

const std::string ROBOT_COMMAND_TORQUES_PREFIX =
	"sai2::interfaces::simviz::robot_command_torques::";
const std::string ROBOT_Q_PREFIX = "sai2::interfaces::simviz::robot_q::";
const std::string ROBOT_DQ_PREFIX = "sai2::interfaces::simviz::robot_dq::";
const std::string OBJECT_POSE_PREFIX = "sai2::interfaces::simviz::obj_pose::";
const std::string OBJECT_VELOCITY_PREFIX =
	"sai2::interfaces::simviz::obj_velocity::";
const std::string FORCE_SENSOR_PREFIX =
	"sai2::interfaces::simviz::force_sensor::";
}  // namespace

SimVizRedisInterface::SimVizRedisInterface(const std::string& config_file)
	: _config_file(config_file),
	  _pause(false),
	  _reset(false),
	  _enable_grav_comp(false){

	_config = _config_parser.parseConfig(_config_file);
	_graphics = std::make_unique<Sai2Graphics::Sai2Graphics>(
		_config.world_file, "sai2 world", false);
	_simulation = std::make_unique<Sai2Simulation::Sai2Simulation>(
		_config.world_file, false);

	_redis_client.connect();
	_redis_client.createNewSendGroup(group_name);
	_redis_client.createNewReceiveGroup(group_name);
	initializeRedisDatabase();
	reset();

	signal(SIGABRT, &stop);
	signal(SIGTERM, &stop);
	signal(SIGINT, &stop);
}

void SimVizRedisInterface::reset() {

	_simulation->setTimestep(_config.timestep);
	_simulation->setCollisionRestitution(_config.collision_restitution);
	_simulation->setCoeffFrictionStatic(_config.friction_coefficient);

	_robot_ui_torques.clear();
	_object_ui_torques.clear();
	_redis_client.deleteSendGroup(group_name);
	_redis_client.deleteReceiveGroup(group_name);
	_redis_client.createNewSendGroup(group_name);
	_redis_client.createNewReceiveGroup(group_name);

	_robot_control_torques.clear();
	_robot_q.clear();
	_robot_dq.clear();
	_object_pose.clear();
	_object_vel.clear();
	_force_sensor_data.clear();

	for (auto& robot_name : _simulation->getRobotNames()) {
		if(_config.enable_joint_limits) {
			_simulation->enableJointLimits(robot_name);
		} else {
			_simulation->disableJointLimits(robot_name);
		}

		const int robot_dof = _simulation->dof(robot_name);
		_graphics->updateRobotGraphics(
			robot_name, _simulation->getJointPositions(robot_name));
		_graphics->addUIForceInteraction(robot_name);
		_robot_ui_torques[robot_name] = Eigen::VectorXd::Zero(robot_dof);

		_robot_control_torques[robot_name] = VectorXd::Zero(robot_dof);
		_robot_q[robot_name] = VectorXd::Zero(robot_dof);
		_robot_dq[robot_name] = VectorXd::Zero(robot_dof);

		_redis_client.addToReceiveGroup(
			ROBOT_COMMAND_TORQUES_PREFIX + robot_name,
			_robot_control_torques.at(robot_name), group_name);
		_redis_client.addToSendGroup(ROBOT_Q_PREFIX + robot_name,
									 _robot_q.at(robot_name), group_name);
		_redis_client.addToSendGroup(ROBOT_DQ_PREFIX + robot_name,
									 _robot_dq.at(robot_name), group_name);
	}

	for (auto& object_name : _simulation->getObjectNames()) {
		_object_pose[object_name] = Affine3d::Identity().matrix();
		_object_vel[object_name] = Vector6d::Zero();

		_redis_client.addToSendGroup(OBJECT_POSE_PREFIX + object_name,
									 _object_pose.at(object_name), group_name);
		_redis_client.addToSendGroup(OBJECT_VELOCITY_PREFIX + object_name,
									 _object_vel.at(object_name), group_name);

		_graphics->addUIForceInteraction(object_name);
		_object_ui_torques[object_name] = Eigen::VectorXd::Zero(6);
	}

	for (const auto& force_sensor_config : _config.force_sensors) {
		_simulation->addSimulatedForceSensor(force_sensor_config.robot_name,
									force_sensor_config.link_name,
									force_sensor_config.transform_in_link,
									force_sensor_config.cutoff_frequency);
	}

	_force_sensor_data = _simulation->getAllForceSensorData();
	for (auto& force_sensor_data : _force_sensor_data) {
		_graphics->addForceSensorDisplay(force_sensor_data);
		_redis_client.addToSendGroup(
			FORCE_SENSOR_PREFIX + force_sensor_data.robot_name +
				"::" + force_sensor_data.link_name + "::force",
			force_sensor_data.force_local_frame, group_name);
		_redis_client.addToSendGroup(
			FORCE_SENSOR_PREFIX + force_sensor_data.robot_name +
				"::" + force_sensor_data.link_name + "::moment",
			force_sensor_data.moment_local_frame, group_name);
	}
}

void SimVizRedisInterface::initializeRedisDatabase() {
	_redis_client.setInt(SIM_PAUSE_KEY, _pause);
	_redis_client.setInt(SIM_RESET_KEY, _reset);
	_redis_client.setInt(GRAV_COMP_ENABLED_KEY, _enable_grav_comp);
	_redis_client.set(CONFIG_FILE_KEY, _config_file);
}

void SimVizRedisInterface::run() {
	std::thread sim_thread(&SimVizRedisInterface::simLoopRun, this);
	vizLoopRun();
	sim_thread.join();
}

void SimVizRedisInterface::vizLoopRun() {
	Sai2Common::LoopTimer timer(60.0);

	while (!should_stop && _graphics->isWindowOpen()) {
		usleep(10);
		timer.waitForNextLoop();

		std::lock_guard<std::mutex> lock(_mutex_parametrization);
		for (auto& robot_name : _simulation->getRobotNames()) {
			_graphics->updateRobotGraphics(robot_name, _robot_q.at(robot_name));
		}
		for (auto& object_name : _simulation->getObjectNames()) {
			_graphics->updateObjectGraphics(
				object_name, Eigen::Affine3d(_object_pose.at(object_name)),
				_object_vel.at(object_name));
		}
		for (auto& force_sensor_data : _force_sensor_data) {
			_graphics->updateDisplayedForceSensor(force_sensor_data);
		}
		_graphics->renderGraphicsWorld();
		for (auto& robot_name : _simulation->getRobotNames()) {
			std::lock_guard<std::mutex> lock(_mutex_torques);
			_robot_ui_torques.at(robot_name) = _graphics->getUITorques(robot_name);
		}
		for (auto& object_name : _simulation->getObjectNames()) {
			std::lock_guard<std::mutex> lock(_mutex_torques);
			_object_ui_torques.at(object_name) = _graphics->getUITorques(object_name);
		}


	}
	should_stop = true;
}

void SimVizRedisInterface::simLoopRun() {
	Sai2Common::LoopTimer timer(1.0 / _simulation->timestep());

	while (!should_stop) {
		getSimParametrization();
		if (_reset) {
			timer.resetLoopFrequency(1.0 / _simulation->timestep());
			timer.reinitializeTimer();
		}
		processSimParametrization();

		_reset = false;
		_redis_client.setInt(SIM_RESET_KEY, _reset);

		if (_pause) {
			timer.stop();
			_simulation->pause();
		} else {
			if (_simulation->isPaused()) {
				_simulation->unpause();
				timer.reinitializeTimer();
			}
			timer.waitForNextLoop();
			_redis_client.receiveAllFromGroup(group_name);
			// readInputs();
			for (auto& robot_name : _simulation->getRobotNames()) {
				std::lock_guard<std::mutex> lock(_mutex_torques);
				_simulation->setJointTorques(
					robot_name, _robot_ui_torques.at(robot_name) +
									_robot_control_torques.at(robot_name));
			}
			for (auto& object_name : _simulation->getObjectNames()) {
				std::lock_guard<std::mutex> lock(_mutex_torques);
				_simulation->setObjectForceTorque(
					object_name, _object_ui_torques.at(object_name));
			}

			_simulation->integrate();

			for (auto& robot_name : _simulation->getRobotNames()) {
				_robot_q.at(robot_name) =
					_simulation->getJointPositions(robot_name);
				_robot_dq.at(robot_name) =
					_simulation->getJointVelocities(robot_name);
			}
			for (auto& object_name : _simulation->getObjectNames()) {
				_object_pose.at(object_name) =
					_simulation->getObjectPose(object_name).matrix();
				_object_vel.at(object_name) =
					_simulation->getObjectVelocity(object_name);
			}
			_force_sensor_data = _simulation->getAllForceSensorData();
			_redis_client.sendAllFromGroup(group_name);
			// writeOutputs();
		}
	}
	timer.stop();
	cout << "\nSimulation loop timer stats:\n";
	timer.printInfoPostRun();
}

void SimVizRedisInterface::getSimParametrization() {
	_pause = _redis_client.getInt(SIM_PAUSE_KEY);
	_reset = _redis_client.getInt(SIM_RESET_KEY);
	_enable_grav_comp = _redis_client.getInt(GRAV_COMP_ENABLED_KEY);
	_config_file = _redis_client.get(CONFIG_FILE_KEY);
}

void SimVizRedisInterface::processSimParametrization() {
	if (_reset) {
		std::lock_guard<mutex> lock(_mutex_parametrization);
		_config = _config_parser.parseConfig(_config_file);
		_simulation->resetWorld(_config.world_file);
		_graphics->resetWorld(_config.world_file);
		reset();
	}

	if (_enable_grav_comp != _simulation->isGravityCompensationEnabled()) {
		_simulation->enableGravityCompensation(_enable_grav_comp);
	}
}

void SimVizRedisInterface::readInputs() {}
void SimVizRedisInterface::writeOutputs() {}

}  // namespace Sai2Interfaces