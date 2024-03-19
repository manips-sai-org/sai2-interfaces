#ifndef SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H
#define SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H

#include <Eigen/Dense>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "Sai2Graphics.h"
#include "Sai2Simulation.h"
#include "SimVizConfig.h"
#include "logger/Logger.h"
#include "redis/RedisClient.h"
#include "timer/LoopTimer.h"

namespace Sai2Interfaces {

class SimVizRedisInterface {
public:
	SimVizRedisInterface(const SimVizConfig& config);
	~SimVizRedisInterface() = default;

	void setNewConfig(const SimVizConfig& config);
	void run(const std::atomic<bool>& user_stop_signal = false);

private:
	enum LoggingState {
		OFF,
		START,
		ON,
		STOP,
	};

	void resetInternal();
	void initializeRedisDatabase();

	void vizLoopRun(const std::atomic<bool>& user_stop_signal);
	void simLoopRun(const std::atomic<bool>& user_stop_signal);
	void processSimParametrization();

	SimVizConfig _config;
	SimVizConfig _new_config;

	Sai2Common::RedisClient _redis_client;

	std::unique_ptr<Sai2Graphics::Sai2Graphics> _graphics;
	std::unique_ptr<Sai2Simulation::Sai2Simulation> _simulation;

	std::map<std::string, Eigen::VectorXd> _robot_ui_torques;
	std::map<std::string, Eigen::VectorXd> _object_ui_torques;

	std::map<std::string, Eigen::VectorXd> _robot_control_torques;
	std::map<std::string, Eigen::VectorXd> _robot_q;
	std::map<std::string, Eigen::VectorXd> _robot_dq;
	std::map<std::string, Eigen::Matrix4d> _object_pose;
	std::map<std::string, Eigen::Vector6d> _object_vel;

	std::vector<Sai2Model::ForceSensorData> _force_sensor_data;

	std::map<std::string, std::unique_ptr<Sai2Common::Logger>> _loggers;

	std::mutex _mutex_parametrization;
	std::mutex _mutex_torques;

	LoggingState _logging_state;
	bool _logging_on;
	bool _pause;
	bool _reset;
	bool _reset_config;
	bool _enable_grav_comp;
};

}  // namespace Sai2Interfaces

#endif /* SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H */