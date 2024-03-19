#include "RobotControllerRedisInterface.h"

#include <signal.h>

#include "timer/LoopTimer.h"

using namespace std;
using namespace Eigen;

namespace Sai2Interfaces {

namespace {

bool should_stop = false;
void stop(int i) { should_stop = true; }

const std::string reset_inputs_redis_group = "reset_input_group";

std::string getActiveControllerKey(const std::string& robot_name) {
	return "sai2::interfaces::controller::" + robot_name +
		   "::active_controller_name";
}

std::string getLoggingOnKey(const std::string& robot_name) {
	return "sai2::interfaces::controller::" + robot_name + "::logging_on";
}

const std::string ROBOT_COMMAND_TORQUES_PREFIX =
	"sai2::interfaces::robot_command_torques::";
const std::string ROBOT_Q_PREFIX = "sai2::interfaces::robot_q::";
const std::string ROBOT_DQ_PREFIX = "sai2::interfaces::robot_dq::";
const std::string FORCE_SENSOR_PREFIX = "sai2::interfaces::force_sensor::";
}  // namespace

RobotControllerRedisInterface::RobotControllerRedisInterface(
	const string& config_file) {
	_config_file = config_file;
	_config_parser = RobotControllerConfigParser();
	_config = _config_parser.parseConfig(_config_file);

	_logging_on = _config.logger_config.start_with_logger_on;
	_logging_state = _logging_on ? LoggingState::START : LoggingState::OFF;

	_redis_client.connect();

	initialize();

	signal(SIGABRT, &stop);
	signal(SIGTERM, &stop);
	signal(SIGINT, &stop);
}

void RobotControllerRedisInterface::run() {
	// create timer
	Sai2Common::LoopTimer timer(1.0 / _config.timestep);

	while (!should_stop) {
		timer.waitForNextLoop();

		// switch controller if needed
		switchController(
			_redis_client.get(getActiveControllerKey(_config.robot_name)));

		// read from redis
		_redis_client.receiveAllFromGroup();
		_redis_client.receiveAllFromGroup(_active_controller_name);

		// process inputs
		processInputs();

		// update task models
		_robot_model->setQ(_robot_q);
		_robot_model->setDq(_robot_dq);
		_robot_model->updateModel();
		_robot_controllers.at(_active_controller_name)
			->updateControllerTaskModels();

		// compute torques
		_robot_command_torques
			<< _robot_controllers.at(_active_controller_name)
				   ->computeControlTorques();

		// write to redis
		_redis_client.sendAllFromGroup();
	}

	_redis_client.setEigen(ROBOT_COMMAND_TORQUES_PREFIX + _config.robot_name,
						   Eigen::VectorXd::Zero(_robot_model->dof()));

	timer.printInfoPostRun();
}

void RobotControllerRedisInterface::initialize() {
	_robot_model = make_shared<Sai2Model::Sai2Model>(_config.robot_model_file);
	_robot_model->setTRobotBase(_config.robot_base_in_world);
	_robot_model->setWorldGravity(_config.world_gravity);

	_robot_q.setZero(_robot_model->dof());
	_robot_dq.setZero(_robot_model->dof());
	_robot_command_torques.setZero(_robot_model->dof());

	_redis_client.addToSendGroup(
		ROBOT_COMMAND_TORQUES_PREFIX + _config.robot_name,
		_robot_command_torques);

	_redis_client.addToReceiveGroup(ROBOT_Q_PREFIX + _config.robot_name,
									_robot_q);
	_redis_client.addToReceiveGroup(ROBOT_DQ_PREFIX + _config.robot_name,
									_robot_dq);
	_redis_client.addToReceiveGroup(getLoggingOnKey(_config.robot_name),
									_logging_on);

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	_redis_client.receiveAllFromGroup();
	_robot_model->setQ(_robot_q);
	_robot_model->updateModel();
	_robot_M = _robot_model->M();

	for (const auto& pair : _config.controllers_configs) {
		vector<shared_ptr<Sai2Primitives::TemplateTask>> ordered_tasks_list;

		_is_active_controller[pair.first] = false;

		for (const auto& tasks : pair.second) {
			if (holds_alternative<JointTaskConfig>(tasks)) {
				const auto& joint_task_config = get<JointTaskConfig>(tasks);

				// selection matrix for controlled joints
				MatrixXd S = MatrixXd::Identity(_robot_model->dof(),
												_robot_model->dof());

				if (!joint_task_config.controlled_joint_names.empty()) {
					MatrixXd S = MatrixXd::Zero(
						joint_task_config.controlled_joint_names.size(),
						_robot_model->dof());
					for (int i = 0;
						 i < joint_task_config.controlled_joint_names.size();
						 i++) {
						S(i, _robot_model->jointIndex(
								 joint_task_config.controlled_joint_names[i])) =
							1;
					}
				}

				// create joint task
				auto joint_task = make_shared<Sai2Primitives::JointTask>(
					_robot_model, S, joint_task_config.task_name,
					_config.timestep);

				ordered_tasks_list.push_back(joint_task);

			} else if (holds_alternative<MotionForceTaskConfig>(tasks)) {
				const auto& motion_force_task_config =
					get<MotionForceTaskConfig>(tasks);

				// controlled directions
				vector<Vector3d> controlled_directions_position = {
					Vector3d::UnitX(), Vector3d::UnitY(), Vector3d::UnitZ()};
				vector<Vector3d> controlled_directions_orientation = {
					Vector3d::UnitX(), Vector3d::UnitY(), Vector3d::UnitZ()};
				if (motion_force_task_config.controlled_directions_position
						.has_value()) {
					controlled_directions_position =
						motion_force_task_config.controlled_directions_position
							.value();
				}
				if (motion_force_task_config.controlled_directions_orientation
						.has_value()) {
					controlled_directions_orientation =
						motion_force_task_config
							.controlled_directions_orientation.value();
				}

				// create task
				auto motion_force_task =
					make_shared<Sai2Primitives::MotionForceTask>(
						_robot_model, motion_force_task_config.link_name,
						controlled_directions_position,
						controlled_directions_orientation,
						motion_force_task_config.compliant_frame,
						motion_force_task_config.task_name,
						motion_force_task_config
							.is_parametrization_in_compliant_frame,
						_config.timestep);

				ordered_tasks_list.push_back(motion_force_task);
			}
		}
		_robot_controllers[pair.first] =
			make_unique<Sai2Primitives::RobotController>(_robot_model,
														 ordered_tasks_list);
	}

	if (_robot_controllers.find(_config.initial_active_controller_name) ==
		_robot_controllers.end()) {
		throw std::runtime_error(
			"Initial active controller name does not match any controller in "
			"the config file");
	}

	initializeRedisTasksIO();

	switchController(_config.initial_active_controller_name);
	_redis_client.set(getActiveControllerKey(_config.robot_name),
					  _active_controller_name);
}

void RobotControllerRedisInterface::switchController(
	const std::string& controller_name) {
	if (controller_name == _active_controller_name) {
		return;
	}
	if (_robot_controllers.find(controller_name) == _robot_controllers.end()) {
		cout << "WARNING: Controller name does not match any controller in the "
				"config file"
			 << endl;
		return;
	}
	if (_active_controller_name != "") {
		_is_active_controller.at(_active_controller_name) = false;
	}
	_active_controller_name = controller_name;
	_is_active_controller.at(_active_controller_name) = true;

	_robot_model->setQ(_robot_q);
	_robot_model->setDq(_robot_dq);
	_robot_model->updateModel();
	_robot_controllers.at(_active_controller_name)->reinitializeTasks();

	// reset inputs for new active controller
	for (auto& task_input : _controller_inputs.at(_active_controller_name)) {
		if (holds_alternative<JointTaskInput>(task_input.second)) {
			auto& joint_task_input = get<JointTaskInput>(task_input.second);
			joint_task_input.setFromTask(
				_robot_controllers.at(_active_controller_name)
					->getJointTaskByName(task_input.first));
		} else if (holds_alternative<MotionForceTaskInput>(task_input.second)) {
			auto& motion_force_task_input =
				get<MotionForceTaskInput>(task_input.second);
			motion_force_task_input.setFromTask(
				_robot_controllers.at(_active_controller_name)
					->getMotionForceTaskByName(task_input.first));
		}
	}
	_redis_client.sendAllFromGroup(reset_inputs_redis_group);
}

void RobotControllerRedisInterface::initializeRedisTasksIO() {
	_redis_client.createNewSendGroup(reset_inputs_redis_group);

	if (!std::filesystem::exists(_config.logger_config.folder_name)) {
		std::filesystem::create_directory(_config.logger_config.folder_name);
	}

	_robot_logger = std::make_unique<Sai2Common::Logger>(
		_config.logger_config.folder_name + '/' + _config.robot_name,
		_config.logger_config.add_timestamp_to_filename);

	_robot_logger->addToLog(_robot_q, "q");
	_robot_logger->addToLog(_robot_dq, "dq");
	_robot_logger->addToLog(_robot_command_torques, "command_torques");
	_robot_logger->addToLog(_robot_M, "Mass_Matrix");

	for (auto& pair : _config.controllers_configs) {
		const string& controller_name = pair.first;

		_controller_inputs[controller_name] = {};
		_controller_task_monitoring_data[controller_name] = {};
		_redis_client.createNewReceiveGroup(controller_name);

		if (!std::filesystem::exists(_config.logger_config.folder_name + '/' +
									 controller_name)) {
			std::filesystem::create_directory(
				_config.logger_config.folder_name + '/' + controller_name);
		}

		auto& task_configs = pair.second;

		for (auto& task_config : task_configs) {
			if (holds_alternative<JointTaskConfig>(task_config)) {
				auto& joint_task_config = get<JointTaskConfig>(task_config);
				const string task_name = joint_task_config.task_name;
				auto joint_task = _robot_controllers.at(controller_name)
									  ->getJointTaskByName(task_name);
				_controller_inputs.at(controller_name)[task_name] =
					JointTaskInput(joint_task->getTaskDof());
				_controller_task_monitoring_data.at(
					controller_name)[task_name] =
					JointTaskMonitoringData(joint_task->getTaskDof());

				_task_loggers[controller_name][task_name] =
					std::make_unique<Sai2Common::Logger>(
						_config.logger_config.folder_name + '/' +
							controller_name + '/' + task_name,
						_config.logger_config.add_timestamp_to_filename);
				auto task_logger =
					_task_loggers.at(controller_name).at(task_name).get();
				task_logger->addToLog(_is_active_controller.at(controller_name),
									  "is_active");

				const string& key_prefix =
					"sai2::interfaces::controller::" + _config.robot_name +
					"::" + controller_name + "::" + task_name + "::";

				// dynamic decoupling
				task_logger->addToLog(joint_task_config.use_dynamic_decoupling,
									  "use_dynamic_decoupling");
				_redis_client.addToReceiveGroup(
					key_prefix + "use_dynamic_decoupling",
					joint_task_config.use_dynamic_decoupling, controller_name);

				// gains
				if (!joint_task_config.gains_config.has_value()) {
					joint_task_config.gains_config = GainsConfig(
						JointTaskDefaultParams::kp, JointTaskDefaultParams::kv,
						JointTaskDefaultParams::ki);
				}
				task_logger->addToLog(joint_task_config.gains_config->kp, "kp");
				task_logger->addToLog(joint_task_config.gains_config->kv, "kv");
				task_logger->addToLog(joint_task_config.gains_config->ki, "ki");
				_redis_client.addToReceiveGroup(
					key_prefix + "kp", joint_task_config.gains_config->kp,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "kv", joint_task_config.gains_config->kv,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "ki", joint_task_config.gains_config->ki,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "gains_safety_checks_enabled",
					joint_task_config.gains_config->safety_checks_enabled,
					controller_name);

				// velocity saturation
				if (!joint_task_config.velocity_saturation_config.has_value()) {
					joint_task_config.velocity_saturation_config =
						JointTaskConfig::JointVelSatConfig();
				}
				task_logger->addToLog(
					joint_task_config.velocity_saturation_config->enabled,
					"velocity_saturation_enabled");
				task_logger->addToLog(
					joint_task_config.velocity_saturation_config
						->velocity_limits,
					"velocity_saturation_limits");
				_redis_client.addToReceiveGroup(
					key_prefix + "velocity_saturation_enabled",
					joint_task_config.velocity_saturation_config->enabled,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "velocity_saturation_limits",
					joint_task_config.velocity_saturation_config
						->velocity_limits,
					controller_name);

				// otg
				if (!joint_task_config.otg_config.has_value()) {
					joint_task_config.otg_config =
						JointTaskConfig::JointOTGConfig();
				}
				task_logger->addToLog(joint_task_config.otg_config->enabled,
									  "otg_enabled");
				task_logger->addToLog(
					joint_task_config.otg_config->jerk_limited,
					"otg_jerk_limited");
				task_logger->addToLog(
					joint_task_config.otg_config->limits.velocity_limit,
					"otg_velocity_limit");
				task_logger->addToLog(
					joint_task_config.otg_config->limits.acceleration_limit,
					"otg_acceleration_limit");
				task_logger->addToLog(
					joint_task_config.otg_config->limits.jerk_limit,
					"otg_jerk_limit");
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_enabled",
					joint_task_config.otg_config->enabled, controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_jerk_limited",
					joint_task_config.otg_config->jerk_limited,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_velocity_limit",
					joint_task_config.otg_config->limits.velocity_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_acceleration_limit",
					joint_task_config.otg_config->limits.acceleration_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_jerk_limit",
					joint_task_config.otg_config->limits.jerk_limit,
					controller_name);

				// inputs
				JointTaskInput& joint_task_input = std::get<JointTaskInput>(
					_controller_inputs.at(controller_name).at(task_name));
				joint_task_input.setFromTask(joint_task);
				task_logger->addToLog(joint_task_input.goal_position,
									  "goal_position");
				task_logger->addToLog(joint_task_input.goal_velocity,
									  "goal_velocity");
				task_logger->addToLog(joint_task_input.goal_acceleration,
									  "goal_acceleration");
				_redis_client.addToSendGroup(key_prefix + "goal_position",
											 joint_task_input.goal_position,
											 reset_inputs_redis_group);
				_redis_client.addToSendGroup(key_prefix + "goal_velocity",
											 joint_task_input.goal_velocity,
											 reset_inputs_redis_group);
				_redis_client.addToSendGroup(key_prefix + "goal_acceleration",
											 joint_task_input.goal_acceleration,
											 reset_inputs_redis_group);
				_redis_client.addToReceiveGroup(key_prefix + "goal_position",
												joint_task_input.goal_position,
												controller_name);
				_redis_client.addToReceiveGroup(key_prefix + "goal_velocity",
												joint_task_input.goal_velocity,
												controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_acceleration",
					joint_task_input.goal_acceleration, controller_name);

				// monitoring data
				JointTaskMonitoringData& joint_task_monitoring_data =
					std::get<JointTaskMonitoringData>(
						_controller_task_monitoring_data.at(controller_name)
							.at(task_name));
				task_logger->addToLog(
					joint_task_monitoring_data.current_position,
					"current_position");
				task_logger->addToLog(
					joint_task_monitoring_data.current_velocity,
					"current_velocity");
				task_logger->addToLog(
					joint_task_monitoring_data.desired_position,
					"desired_position");
				task_logger->addToLog(
					joint_task_monitoring_data.desired_velocity,
					"desired_velocity");
				task_logger->addToLog(
					joint_task_monitoring_data.desired_acceleration,
					"desired_acceleration");
				_redis_client.addToSendGroup(
					key_prefix + "current_position",
					joint_task_monitoring_data.current_position);
				_redis_client.addToSendGroup(
					key_prefix + "current_velocity",
					joint_task_monitoring_data.current_velocity);

			} else if (holds_alternative<MotionForceTaskConfig>(task_config)) {
				auto& motion_force_task_config =
					get<MotionForceTaskConfig>(task_config);
				const string task_name = motion_force_task_config.task_name;

				auto motion_force_task =
					_robot_controllers.at(controller_name)
						->getMotionForceTaskByName(task_name);

				_controller_inputs.at(controller_name)[task_name] =
					MotionForceTaskInput();
				_controller_task_monitoring_data.at(
					controller_name)[task_name] =
					MotionForceTaskMonitoringData();

				_task_loggers[controller_name][task_name] =
					std::make_unique<Sai2Common::Logger>(
						_config.logger_config.folder_name + '/' +
							controller_name + '/' + task_name,
						_config.logger_config.add_timestamp_to_filename);
				auto task_logger =
					_task_loggers.at(controller_name).at(task_name).get();
				task_logger->addToLog(_is_active_controller.at(controller_name),
									  "is_active");

				const string key_prefix =
					"sai2::interfaces::controller::" + _config.robot_name +
					"::" + controller_name + "::" + task_name + "::";

				// dynamic decoupling
				task_logger->addToLog(
					motion_force_task_config.use_dynamic_decoupling,
					"use_dynamic_decoupling");
				_redis_client.addToReceiveGroup(
					key_prefix + "use_dynamic_decoupling",
					motion_force_task_config.use_dynamic_decoupling,
					controller_name);

				// force control parametrization
				if (!motion_force_task_config.force_space_param_config
						 .has_value()) {
					motion_force_task_config.force_space_param_config =
						MotionForceTaskConfig::ForceMotionSpaceParamConfig();
				}
				if (!motion_force_task_config.moment_space_param_config
						 .has_value()) {
					motion_force_task_config.moment_space_param_config =
						MotionForceTaskConfig::ForceMotionSpaceParamConfig();
				}
				task_logger->addToLog(
					motion_force_task_config.force_space_param_config
						->force_space_dimension,
					"force_space_dimension");
				task_logger->addToLog(
					motion_force_task_config.force_space_param_config->axis,
					"force_or_motion_axis");
				task_logger->addToLog(
					motion_force_task_config.moment_space_param_config
						->force_space_dimension,
					"moment_space_dimension");
				task_logger->addToLog(
					motion_force_task_config.moment_space_param_config->axis,
					"moment_or_rotmotion_axis");
				_redis_client.addToReceiveGroup(
					key_prefix + "closed_loop_force_control",
					motion_force_task_config.closed_loop_force_control,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "force_space_dimension",
					motion_force_task_config.force_space_param_config
						->force_space_dimension,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "force_space_axis",
					motion_force_task_config.force_space_param_config->axis,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "moment_space_dimension",
					motion_force_task_config.moment_space_param_config
						->force_space_dimension,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "moment_space_axis",
					motion_force_task_config.moment_space_param_config->axis,
					controller_name);

				// velocity saturation
				if (!motion_force_task_config.velocity_saturation_config
						 .has_value()) {
					motion_force_task_config.velocity_saturation_config =
						MotionForceTaskConfig::VelSatConfig();
				}
				task_logger->addToLog(motion_force_task_config
										  .velocity_saturation_config->enabled,
									  "velocity_saturation_enabled");
				task_logger->addToLog(
					motion_force_task_config.velocity_saturation_config
						->linear_velocity_limits,
					"linear_velocity_saturation_limits");
				task_logger->addToLog(
					motion_force_task_config.velocity_saturation_config
						->angular_velocity_limits,
					"angular_velocity_saturation_limits");
				_redis_client.addToReceiveGroup(
					key_prefix + "velocity_saturation_enabled",
					motion_force_task_config.velocity_saturation_config
						->enabled,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "linear_velocity_saturation_limits",
					motion_force_task_config.velocity_saturation_config
						->linear_velocity_limits,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "angular_velocity_saturation_limits",
					motion_force_task_config.velocity_saturation_config
						->angular_velocity_limits,
					controller_name);

				// otg
				if (!motion_force_task_config.otg_config.has_value()) {
					motion_force_task_config.otg_config =
						MotionForceTaskConfig::OTGConfig();
				}
				task_logger->addToLog(
					motion_force_task_config.otg_config->enabled,
					"otg_enabled");
				task_logger->addToLog(
					motion_force_task_config.otg_config->jerk_limited,
					"otg_jerk_limited");
				task_logger->addToLog(
					motion_force_task_config.otg_config->linear_velocity_limit,
					"otg_linear_velocity_limit");
				task_logger->addToLog(
					motion_force_task_config.otg_config->angular_velocity_limit,
					"otg_angular_velocity_limit");
				task_logger->addToLog(motion_force_task_config.otg_config
										  ->linear_acceleration_limit,
									  "otg_linear_acceleration_limit");
				task_logger->addToLog(motion_force_task_config.otg_config
										  ->angular_acceleration_limit,
									  "otg_angular_acceleration_limit");
				task_logger->addToLog(
					motion_force_task_config.otg_config->linear_jerk_limit,
					"otg_linear_jerk_limit");
				task_logger->addToLog(
					motion_force_task_config.otg_config->angular_jerk_limit,
					"otg_angular_jerk_limit");
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_enabled",
					motion_force_task_config.otg_config->enabled,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_jerk_limited",
					motion_force_task_config.otg_config->jerk_limited,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_linear_velocity_limit",
					motion_force_task_config.otg_config->linear_velocity_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_angular_velocity_limit",
					motion_force_task_config.otg_config->angular_velocity_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_linear_acceleration_limit",
					motion_force_task_config.otg_config
						->linear_acceleration_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_angular_acceleration_limit",
					motion_force_task_config.otg_config
						->angular_acceleration_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_linear_jerk_limit",
					motion_force_task_config.otg_config->linear_jerk_limit,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "otg_angular_jerk_limit",
					motion_force_task_config.otg_config->angular_jerk_limit,
					controller_name);

				// gains
				if (!motion_force_task_config.position_gains_config
						 .has_value()) {
					motion_force_task_config.position_gains_config =
						GainsConfig(MotionForceTaskDefaultParams::kp_pos,
									MotionForceTaskDefaultParams::kv_pos,
									MotionForceTaskDefaultParams::ki_pos);
				}
				if (!motion_force_task_config.orientation_gains_config
						 .has_value()) {
					motion_force_task_config.orientation_gains_config =
						GainsConfig(MotionForceTaskDefaultParams::kp_ori,
									MotionForceTaskDefaultParams::kv_ori,
									MotionForceTaskDefaultParams::ki_ori);
				}
				if (!motion_force_task_config.force_gains_config.has_value()) {
					motion_force_task_config.force_gains_config =
						GainsConfig(MotionForceTaskDefaultParams::kp_force,
									MotionForceTaskDefaultParams::kv_force,
									MotionForceTaskDefaultParams::ki_force);
				}
				if (!motion_force_task_config.moment_gains_config.has_value()) {
					motion_force_task_config.moment_gains_config =
						GainsConfig(MotionForceTaskDefaultParams::kp_moment,
									MotionForceTaskDefaultParams::kv_moment,
									MotionForceTaskDefaultParams::ki_moment);
				}
				task_logger->addToLog(
					motion_force_task_config.position_gains_config->kp,
					"position_kp");
				task_logger->addToLog(
					motion_force_task_config.position_gains_config->kv,
					"position_kv");
				task_logger->addToLog(
					motion_force_task_config.position_gains_config->ki,
					"position_ki");
				task_logger->addToLog(
					motion_force_task_config.orientation_gains_config->kp,
					"orientation_kp");
				task_logger->addToLog(
					motion_force_task_config.orientation_gains_config->kv,
					"orientation_kv");
				task_logger->addToLog(
					motion_force_task_config.orientation_gains_config->ki,
					"orientation_ki");
				task_logger->addToLog(
					motion_force_task_config.force_gains_config->kp,
					"force_kp");
				task_logger->addToLog(
					motion_force_task_config.force_gains_config->kv,
					"force_kv");
				task_logger->addToLog(
					motion_force_task_config.force_gains_config->ki,
					"force_ki");
				task_logger->addToLog(
					motion_force_task_config.moment_gains_config->kp,
					"moment_kp");
				task_logger->addToLog(
					motion_force_task_config.moment_gains_config->kv,
					"moment_kv");
				task_logger->addToLog(
					motion_force_task_config.moment_gains_config->ki,
					"moment_ki");
				_redis_client.addToReceiveGroup(
					key_prefix + "position_kp",
					motion_force_task_config.position_gains_config->kp,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "position_kv",
					motion_force_task_config.position_gains_config->kv,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "position_ki",
					motion_force_task_config.position_gains_config->ki,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "position_gains_safety_checks_enabled",
					motion_force_task_config.position_gains_config
						->safety_checks_enabled,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "orientation_kp",
					motion_force_task_config.orientation_gains_config->kp,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "orientation_kv",
					motion_force_task_config.orientation_gains_config->kv,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "orientation_ki",
					motion_force_task_config.orientation_gains_config->ki,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "orientation_gains_safety_checks_enabled",
					motion_force_task_config.orientation_gains_config
						->safety_checks_enabled,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "force_kp",
					motion_force_task_config.force_gains_config->kp,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "force_kv",
					motion_force_task_config.force_gains_config->kv,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "force_ki",
					motion_force_task_config.force_gains_config->ki,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "moment_kp",
					motion_force_task_config.moment_gains_config->kp,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "moment_kv",
					motion_force_task_config.moment_gains_config->kv,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "moment_ki",
					motion_force_task_config.moment_gains_config->ki,
					controller_name);

				// input
				MotionForceTaskInput& motion_force_task_input =
					std::get<MotionForceTaskInput>(
						_controller_inputs.at(controller_name).at(task_name));
				motion_force_task_input.setFromTask(motion_force_task);
				task_logger->addToLog(motion_force_task_input.goal_position,
									  "goal_position");
				task_logger->addToLog(
					motion_force_task_input.goal_linear_velocity,
					"goal_linear_velocity");
				task_logger->addToLog(
					motion_force_task_input.goal_linear_acceleration,
					"goal_linear_acceleration");
				task_logger->addToLog(motion_force_task_input.goal_orientation,
									  "goal_orientation");
				task_logger->addToLog(
					motion_force_task_input.goal_angular_velocity,
					"goal_angular_velocity");
				task_logger->addToLog(
					motion_force_task_input.goal_angular_acceleration,
					"goal_angular_acceleration");
				task_logger->addToLog(motion_force_task_input.desired_force,
									  "desired_force");
				task_logger->addToLog(motion_force_task_input.desired_moment,
									  "desired_moment");
				task_logger->addToLog(
					motion_force_task_input.sensed_force_sensor_frame,
					"sensed_force_sensor_frame");
				task_logger->addToLog(
					motion_force_task_input.sensed_moment_sensor_frame,
					"sensed_moment_sensor_frame");
				_redis_client.addToSendGroup(
					key_prefix + "goal_position",
					motion_force_task_input.goal_position,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "goal_linear_velocity",
					motion_force_task_input.goal_linear_velocity,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "goal_linear_acceleration",
					motion_force_task_input.goal_linear_acceleration,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "goal_orientation",
					motion_force_task_input.goal_orientation,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "goal_angular_velocity",
					motion_force_task_input.goal_angular_velocity,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "goal_angular_acceleration",
					motion_force_task_input.goal_angular_acceleration,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "desired_force",
					motion_force_task_input.desired_force,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					key_prefix + "desired_moment",
					motion_force_task_input.desired_moment,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					FORCE_SENSOR_PREFIX + _config.robot_name +
						"::" + motion_force_task_config.link_name + "::force",
					motion_force_task_input.sensed_force_sensor_frame,
					reset_inputs_redis_group);
				_redis_client.addToSendGroup(
					FORCE_SENSOR_PREFIX + _config.robot_name +
						"::" + motion_force_task_config.link_name + "::moment",
					motion_force_task_input.sensed_moment_sensor_frame,
					reset_inputs_redis_group);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_position",
					motion_force_task_input.goal_position, controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_linear_velocity",
					motion_force_task_input.goal_linear_velocity,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_linear_acceleration",
					motion_force_task_input.goal_linear_acceleration,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_orientation",
					motion_force_task_input.goal_orientation, controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_angular_velocity",
					motion_force_task_input.goal_angular_velocity,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "goal_angular_acceleration",
					motion_force_task_input.goal_angular_acceleration,
					controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "desired_force",
					motion_force_task_input.desired_force, controller_name);
				_redis_client.addToReceiveGroup(
					key_prefix + "desired_moment",
					motion_force_task_input.desired_moment, controller_name);
				_redis_client.addToReceiveGroup(
					FORCE_SENSOR_PREFIX + _config.robot_name +
						"::" + motion_force_task_config.link_name + "::force",
					motion_force_task_input.sensed_force_sensor_frame,
					controller_name);
				_redis_client.addToReceiveGroup(
					FORCE_SENSOR_PREFIX + _config.robot_name +
						"::" + motion_force_task_config.link_name + "::moment",
					motion_force_task_input.sensed_moment_sensor_frame,
					controller_name);

				// monitoring data
				MotionForceTaskMonitoringData&
					motion_force_task_monitoring_data =
						std::get<MotionForceTaskMonitoringData>(
							_controller_task_monitoring_data.at(controller_name)
								.at(task_name));
				task_logger->addToLog(
					motion_force_task_monitoring_data.current_position,
					"current_position");
				task_logger->addToLog(
					motion_force_task_monitoring_data.current_orientation,
					"current_orientation");
				task_logger->addToLog(
					motion_force_task_monitoring_data.current_linear_velocity,
					"current_linear_velocity");
				task_logger->addToLog(
					motion_force_task_monitoring_data.current_angular_velocity,
					"current_angular_velocity");
				task_logger->addToLog(
					motion_force_task_monitoring_data.desired_position,
					"desired_position");
				task_logger->addToLog(
					motion_force_task_monitoring_data.desired_orientation,
					"desired_orientation");
				task_logger->addToLog(
					motion_force_task_monitoring_data.desired_linear_velocity,
					"desired_linear_velocity");
				task_logger->addToLog(
					motion_force_task_monitoring_data.desired_angular_velocity,
					"desired_angular_velocity");
				task_logger->addToLog(motion_force_task_monitoring_data
										  .desired_linear_acceleration,
									  "desired_linear_acceleration");
				task_logger->addToLog(motion_force_task_monitoring_data
										  .desired_angular_acceleration,
									  "desired_angular_acceleration");
				task_logger->addToLog(
					motion_force_task_monitoring_data.sensed_force_world_frame,
					"sensed_force_world_frame");
				task_logger->addToLog(
					motion_force_task_monitoring_data.sensed_moment_world_frame,
					"sensed_moment_world_frame");
				_redis_client.addToSendGroup(
					key_prefix + "sensed_force",
					motion_force_task_monitoring_data.sensed_force_world_frame);
				_redis_client.addToSendGroup(key_prefix + "sensed_moment",
											 motion_force_task_monitoring_data
												 .sensed_moment_world_frame);
				_redis_client.addToSendGroup(
					key_prefix + "current_position",
					motion_force_task_monitoring_data.current_position);
				_redis_client.addToSendGroup(
					key_prefix + "current_orientation",
					motion_force_task_monitoring_data.current_orientation);
				_redis_client.addToSendGroup(
					key_prefix + "current_linear_velocity",
					motion_force_task_monitoring_data.current_linear_velocity);
				_redis_client.addToSendGroup(
					key_prefix + "current_angular_velocity",
					motion_force_task_monitoring_data.current_angular_velocity);
			}
		}
	}
}

void RobotControllerRedisInterface::processInputs() {
	bool reset_inputs = false;

	auto& current_controller_config =
		_config.controllers_configs.at(_active_controller_name);

	for (auto& task_config : current_controller_config) {
		if (holds_alternative<JointTaskConfig>(task_config)) {
			auto& joint_task_config = get<JointTaskConfig>(task_config);
			auto joint_task =
				_robot_controllers.at(_active_controller_name)
					->getJointTaskByName(joint_task_config.task_name);

			// dynamic decoupling
			if (joint_task_config.use_dynamic_decoupling) {
				joint_task->setDynamicDecouplingType(
					Sai2Primitives::JointTask::DynamicDecouplingType::
						BOUNDED_INERTIA_ESTIMATES);
			} else {
				joint_task->setDynamicDecouplingType(
					Sai2Primitives::JointTask::DynamicDecouplingType::
						IMPEDANCE);
			}

			// velocity saturation
			if (!joint_task_config.velocity_saturation_config.has_value()) {
				auto velocity_saturation_config =
					JointTaskConfig::JointVelSatConfig();
				velocity_saturation_config.enabled =
					joint_task->getVelocitySaturationEnabled();
				velocity_saturation_config.velocity_limits =
					joint_task->getVelocitySaturationMaxVelocity();
				joint_task_config.velocity_saturation_config =
					velocity_saturation_config;
			} else {
				const auto& velocity_saturation_config =
					joint_task_config.velocity_saturation_config.value();
				if (velocity_saturation_config.enabled) {
					joint_task->enableVelocitySaturation(
						velocity_saturation_config.velocity_limits);
				} else {
					joint_task->disableVelocitySaturation();
				}
			}

			// otg
			if (!joint_task_config.otg_config.has_value()) {
				auto otg_config = JointTaskConfig::JointOTGConfig();
				otg_config.enabled = joint_task->getInternalOtgEnabled();
				otg_config.jerk_limited =
					joint_task->getInternalOtg().getJerkLimitEnabled();
				otg_config.limits.velocity_limit =
					joint_task->getInternalOtg().getMaxVelocity();
				otg_config.limits.acceleration_limit =
					joint_task->getInternalOtg().getMaxAcceleration();
				otg_config.limits.jerk_limit =
					joint_task->getInternalOtg().getMaxJerk();
				joint_task_config.otg_config = otg_config;
			} else {
				const auto& otg_config = joint_task_config.otg_config.value();
				if (!otg_config.enabled) {
					joint_task->disableInternalOtg();
				} else if (otg_config.enabled && !otg_config.jerk_limited) {
					joint_task->enableInternalOtgAccelerationLimited(
						otg_config.limits.velocity_limit,
						otg_config.limits.acceleration_limit);
				} else {
					joint_task->enableInternalOtgJerkLimited(
						otg_config.limits.velocity_limit,
						otg_config.limits.acceleration_limit,
						otg_config.limits.jerk_limit);
				}
			}

			// gains
			if (!joint_task_config.gains_config.has_value()) {
				auto gains_config = GainsConfig();
				gains_config.kp = Sai2Primitives::extractKpFromGainVector(
					joint_task->getGains());
				gains_config.kv = Sai2Primitives::extractKvFromGainVector(
					joint_task->getGains());
				gains_config.ki = Sai2Primitives::extractKiFromGainVector(
					joint_task->getGains());
				joint_task_config.gains_config = gains_config;
			} else {
				const auto& gains_config =
					joint_task_config.gains_config.value();
				if (gains_config.safety_checks_enabled) {
					joint_task->setGains(gains_config.kp, gains_config.kv,
										 gains_config.ki);
				} else {
					joint_task->setGainsUnsafe(gains_config.kp, gains_config.kv,
											   gains_config.ki);
				}
			}

			// inputs
			auto& joint_task_input = std::get<JointTaskInput>(
				_controller_inputs.at(_active_controller_name)
					.at(joint_task_config.task_name));
			joint_task->setGoalPosition(joint_task_input.goal_position);
			joint_task->setGoalVelocity(joint_task_input.goal_velocity);
			joint_task->setGoalAcceleration(joint_task_input.goal_acceleration);

			// monitoring data
			auto& joint_task_monitoring_data =
				std::get<JointTaskMonitoringData>(
					_controller_task_monitoring_data.at(_active_controller_name)
						.at(joint_task_config.task_name));
			joint_task_monitoring_data.setFromTask(joint_task);

		} else if (holds_alternative<MotionForceTaskConfig>(task_config)) {
			auto& motion_force_task_config =
				get<MotionForceTaskConfig>(task_config);
			auto motion_force_task =
				_robot_controllers.at(_active_controller_name)
					->getMotionForceTaskByName(
						motion_force_task_config.task_name);
			auto& motion_force_task_input = std::get<MotionForceTaskInput>(
				_controller_inputs.at(_active_controller_name)
					.at(motion_force_task_config.task_name));

			if (motion_force_task_config.use_dynamic_decoupling) {
				motion_force_task->setDynamicDecouplingType(
					Sai2Primitives::MotionForceTask::DynamicDecouplingType::
						BOUNDED_INERTIA_ESTIMATES);
			} else {
				motion_force_task->setDynamicDecouplingType(
					Sai2Primitives::MotionForceTask::DynamicDecouplingType::
						IMPEDANCE);
			}

			// force control parametrization
			motion_force_task->setClosedLoopForceControl(
				motion_force_task_config.closed_loop_force_control);
			motion_force_task->setClosedLoopMomentControl(
				motion_force_task_config.closed_loop_force_control);

			if (!motion_force_task_config.force_space_param_config
					 .has_value()) {
				auto force_space_param_config =
					MotionForceTaskConfig::ForceMotionSpaceParamConfig();
				force_space_param_config.force_space_dimension =
					motion_force_task->getForceSpaceDimension();
				force_space_param_config.axis =
					motion_force_task->getForceMotionSingleAxis();
				motion_force_task_config.force_space_param_config =
					force_space_param_config;
			} else {
				const auto& force_space_param_config =
					motion_force_task_config.force_space_param_config.value();
				if (motion_force_task->parametrizeForceMotionSpaces(
						force_space_param_config.force_space_dimension,
						force_space_param_config.axis)) {
					reset_inputs = true;
					motion_force_task_input.goal_position =
						motion_force_task->getCurrentPosition();
					motion_force_task_input.goal_linear_velocity.setZero();
					motion_force_task_input.goal_linear_acceleration.setZero();
				}
			}

			if (!motion_force_task_config.moment_space_param_config
					 .has_value()) {
				auto moment_space_param_config =
					MotionForceTaskConfig::ForceMotionSpaceParamConfig();
				moment_space_param_config.force_space_dimension =
					motion_force_task->getMomentSpaceDimension();
				moment_space_param_config.axis =
					motion_force_task->getMomentRotMotionSingleAxis();
				motion_force_task_config.moment_space_param_config =
					moment_space_param_config;
			} else {
				const auto& moment_space_param_config =
					motion_force_task_config.moment_space_param_config.value();
				if (motion_force_task->parametrizeMomentRotMotionSpaces(
						moment_space_param_config.force_space_dimension,
						moment_space_param_config.axis)) {
					reset_inputs = true;
					motion_force_task_input.goal_position =
						motion_force_task->getCurrentPosition();
					motion_force_task_input.goal_linear_velocity.setZero();
					motion_force_task_input.goal_linear_acceleration.setZero();
				}
			}

			// velocity saturation
			if (!motion_force_task_config.velocity_saturation_config
					 .has_value()) {
				auto velocity_saturation_config =
					MotionForceTaskConfig::VelSatConfig();
				velocity_saturation_config.enabled =
					motion_force_task->getVelocitySaturationEnabled();
				velocity_saturation_config.linear_velocity_limits =
					motion_force_task->getLinearSaturationVelocity();
				velocity_saturation_config.angular_velocity_limits =
					motion_force_task->getAngularSaturationVelocity();
				motion_force_task_config.velocity_saturation_config =
					velocity_saturation_config;
			} else {
				const auto& velocity_saturation_config =
					motion_force_task_config.velocity_saturation_config.value();
				if (velocity_saturation_config.enabled) {
					motion_force_task->enableVelocitySaturation(
						velocity_saturation_config.linear_velocity_limits,
						velocity_saturation_config.angular_velocity_limits);
				} else {
					motion_force_task->disableVelocitySaturation();
				}
			}

			// otg
			if (!motion_force_task_config.otg_config.has_value()) {
				auto otg_config = MotionForceTaskConfig::OTGConfig();
				otg_config.enabled = motion_force_task->getInternalOtgEnabled();
				otg_config.jerk_limited =
					motion_force_task->getInternalOtg().getJerkLimitEnabled();
				otg_config.linear_velocity_limit =
					motion_force_task->getInternalOtg().getMaxLinearVelocity()(
						0);
				otg_config.angular_velocity_limit =
					motion_force_task->getInternalOtg().getMaxAngularVelocity()(
						0);
				otg_config.linear_acceleration_limit =
					motion_force_task->getInternalOtg()
						.getMaxLinearAcceleration()(0);
				otg_config.angular_acceleration_limit =
					motion_force_task->getInternalOtg()
						.getMaxAngularAcceleration()(0);
				otg_config.linear_jerk_limit =
					motion_force_task->getInternalOtg().getMaxLinearJerk()(0);
				otg_config.angular_jerk_limit =
					motion_force_task->getInternalOtg().getMaxAngularJerk()(0);
				motion_force_task_config.otg_config = otg_config;
			} else {
				const auto& otg_config =
					motion_force_task_config.otg_config.value();
				if (!otg_config.enabled) {
					motion_force_task->disableInternalOtg();
				} else if (otg_config.enabled && !otg_config.jerk_limited) {
					motion_force_task->enableInternalOtgAccelerationLimited(
						otg_config.linear_velocity_limit,
						otg_config.linear_acceleration_limit,
						otg_config.angular_velocity_limit,
						otg_config.angular_acceleration_limit);
				} else {
					motion_force_task->enableInternalOtgJerkLimited(
						otg_config.linear_velocity_limit,
						otg_config.linear_acceleration_limit,
						otg_config.linear_jerk_limit,
						otg_config.angular_velocity_limit,
						otg_config.angular_acceleration_limit,
						otg_config.angular_jerk_limit);
				}
			}

			// gains
			if (!motion_force_task_config.position_gains_config.has_value()) {
				auto gains_config = GainsConfig();
				gains_config.kp = Sai2Primitives::extractKpFromGainVector(
					motion_force_task->getPosControlGains());
				gains_config.kv = Sai2Primitives::extractKvFromGainVector(
					motion_force_task->getPosControlGains());
				gains_config.ki = Sai2Primitives::extractKiFromGainVector(
					motion_force_task->getPosControlGains());
				motion_force_task_config.position_gains_config = gains_config;
			} else {
				const auto& gains_config =
					motion_force_task_config.position_gains_config.value();
				if (gains_config.safety_checks_enabled) {
					motion_force_task->setPosControlGains(
						gains_config.kp, gains_config.kv, gains_config.ki);
				} else {
					motion_force_task->setPosControlGainsUnsafe(
						gains_config.kp, gains_config.kv, gains_config.ki);
				}
			}

			if (!motion_force_task_config.orientation_gains_config
					 .has_value()) {
				auto gains_config = GainsConfig();
				gains_config.kp = Sai2Primitives::extractKpFromGainVector(
					motion_force_task->getOriControlGains());
				gains_config.kv = Sai2Primitives::extractKvFromGainVector(
					motion_force_task->getOriControlGains());
				gains_config.ki = Sai2Primitives::extractKiFromGainVector(
					motion_force_task->getOriControlGains());
				motion_force_task_config.orientation_gains_config =
					gains_config;
			} else {
				const auto& gains_config =
					motion_force_task_config.orientation_gains_config.value();
				if (gains_config.safety_checks_enabled) {
					motion_force_task->setOriControlGains(
						gains_config.kp, gains_config.kv, gains_config.ki);
				} else {
					motion_force_task->setOriControlGainsUnsafe(
						gains_config.kp, gains_config.kv, gains_config.ki);
				}
			}

			if (!motion_force_task_config.force_gains_config.has_value()) {
				auto gains_config = GainsConfig();
				gains_config.kp = Sai2Primitives::extractKpFromGainVector(
					motion_force_task->getForceControlGains());
				gains_config.kv = Sai2Primitives::extractKvFromGainVector(
					motion_force_task->getForceControlGains());
				gains_config.ki = Sai2Primitives::extractKiFromGainVector(
					motion_force_task->getForceControlGains());
				motion_force_task_config.force_gains_config = gains_config;
			} else {
				const auto& gains_config =
					motion_force_task_config.force_gains_config.value();
				motion_force_task->setForceControlGains(
					gains_config.kp(0), gains_config.kv(0), gains_config.ki(0));
			}

			if (!motion_force_task_config.moment_gains_config.has_value()) {
				auto gains_config = GainsConfig();
				gains_config.kp = Sai2Primitives::extractKpFromGainVector(
					motion_force_task->getMomentControlGains());
				gains_config.kv = Sai2Primitives::extractKvFromGainVector(
					motion_force_task->getMomentControlGains());
				gains_config.ki = Sai2Primitives::extractKiFromGainVector(
					motion_force_task->getMomentControlGains());
				motion_force_task_config.moment_gains_config = gains_config;
			} else {
				const auto& gains_config =
					motion_force_task_config.moment_gains_config.value();
				motion_force_task->setMomentControlGains(
					gains_config.kp(0), gains_config.kv(0), gains_config.ki(0));
			}

			// inputs
			motion_force_task->setGoalPosition(
				motion_force_task_input.goal_position);
			motion_force_task->setGoalLinearVelocity(
				motion_force_task_input.goal_linear_velocity);
			motion_force_task->setGoalLinearAcceleration(
				motion_force_task_input.goal_linear_acceleration);
			motion_force_task->setGoalOrientation(
				motion_force_task_input.goal_orientation);
			motion_force_task->setGoalAngularVelocity(
				motion_force_task_input.goal_angular_velocity);
			motion_force_task->setGoalAngularAcceleration(
				motion_force_task_input.goal_angular_acceleration);
			motion_force_task->setGoalForce(
				motion_force_task_input.desired_force);
			motion_force_task->setGoalMoment(
				motion_force_task_input.desired_moment);
			motion_force_task->updateSensedForceAndMoment(
				motion_force_task_input.sensed_force_sensor_frame,
				motion_force_task_input.sensed_moment_sensor_frame);
			if (reset_inputs) {
				_redis_client.sendAllFromGroup(reset_inputs_redis_group);
			}

			// monitoring data
			auto& motion_force_task_monitoring_data =
				std::get<MotionForceTaskMonitoringData>(
					_controller_task_monitoring_data.at(_active_controller_name)
						.at(motion_force_task_config.task_name));
			motion_force_task_monitoring_data.setFromTask(motion_force_task);
		}
	}

	// logging
	switch (_logging_state) {
		case LoggingState::OFF:
			if (_logging_on) {
				_logging_state = LoggingState::START;
			}
			break;
		case LoggingState::START:
			_robot_logger->start(_config.logger_config.frequency);
			for (auto& task_loggers : _task_loggers) {
				for (auto& pair : task_loggers.second) {
					pair.second->start(_config.logger_config.frequency);
				}
			}
			_logging_state = LoggingState::ON;
			break;

		case LoggingState::ON:
			if (!_logging_on) {
				_logging_state = LoggingState::STOP;
			}
			break;

		case LoggingState::STOP:
			_robot_logger->stop();
			for (auto& task_loggers : _task_loggers) {
				for (auto& pair : task_loggers.second) {
					pair.second->stop();
				}
			}
			_logging_state = LoggingState::OFF;
			break;

		default:
			break;
	}
}

}  // namespace Sai2Interfaces