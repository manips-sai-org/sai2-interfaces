#ifndef SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H
#define SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H

#include <Eigen/Dense>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "Sai2Common.h"
#include "Sai2Graphics.h"
#include "Sai2Simulation.h"
#include "SimVizConfig.h"

namespace Sai2Interfaces {

/**
 * @brief Class to run a simulation and visualization from a custom xml config
 * file and provide an easy method of interaction via redis
 *
 */
class SimVizRedisInterface {
public:
	/**
	 * @brief Construct a new SimViz Redis Interface object
	 *
	 * @param config The configuration object for the simulation and
	 * visualization
	 * @param setup_signal_handler Whether to setup the signal handler for the
	 * controller (set to false if a signal handler is already setup elsewhere
	 * in the application)
	 */
	SimVizRedisInterface(const SimVizConfig& config,
						 const bool setup_signal_handler = true);
	~SimVizRedisInterface() = default;

	/**
	 * @brief Reset the simulation and visualization with a new configuration.
	 * Should be called from a different thread than the run function since the
	 * latter is blocking. When called, the run function will handle the reset
	 * and restart the loop with the new config automatically.
	 *
	 * @param config The new configuration object for the simulation and
	 * visualization
	 */
	void reset(const SimVizConfig& config);

	/**
	 * @brief Run the simulation and visualization. This function will run until
	 * the user_stop_signal is set to true externally, or the signal handler is
	 * triggered by ctrl+c if it was setup. It is a blocking finction, and needs
	 * to be called in the main thread of the application.
	 *
	 * @param user_stop_signal
	 */
	void run(const std::atomic<bool>& user_stop_signal = false);

	/**
	 * @brief Function to know if a reset is completed or not. Useful in a
	 * second thread to know if the run function has finished handling the
	 * reset.
	 *
	 * @return true if the reset is complete, false if the reset is still in
	 * progress.
	 */
	bool isResetComplete() const { return _reset_complete; }

	/**
	 * @brief Get the names of all robots in the simulation
	 * 
	 * @return a vector of robot names
	 */
	std::vector<std::string> getRobotNames() const;

	/**
	 * @brief Get the names of all objects in the simulation
	 * 
	 * @return a vector of object names
	 */
	std::vector<std::string> getObjectNames() const;

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
	void redisCommunicationLoopRun(const std::atomic<bool>& user_stop_signal);
	double computeCommunicationLoopFrequency() const;
	void processSimParametrization();

	void setModelSpecificParametersFromConfig(const std::string& model_name);

	SimVizConfig _config;
	SimVizConfig _new_config;

	std::unique_ptr<Sai2Common::RedisClient> _redis_client;

	std::unique_ptr<Sai2Graphics::Sai2Graphics> _graphics;
	std::unique_ptr<Sai2Simulation::Sai2Simulation> _simulation;
	std::unique_ptr<Sai2Common::LoopTimer> _sim_timer, _communication_timer;

	std::map<std::string, Eigen::VectorXd> _robot_ui_torques;
	std::map<std::string, Eigen::VectorXd> _object_ui_torques;

	std::map<std::string, Eigen::VectorXd> _robot_control_torques;
	std::map<std::string, Eigen::VectorXd> _robot_q;
	std::map<std::string, Eigen::VectorXd> _robot_dq;
	std::map<std::string, Eigen::MatrixXd> _robot_M;
	std::map<std::string, Eigen::Matrix4d> _object_pose;
	std::map<std::string, Eigen::Vector6d> _object_vel;

	std::vector<Sai2Model::ForceSensorData> _force_sensor_data;

	std::map<std::string, std::unique_ptr<Sai2Common::Logger>> _loggers;

	std::map<std::string, std::string> _model_specific_params_string;

	std::mutex _mutex_parametrization;
	std::mutex _mutex_torques;

	LoggingState _logging_state;
	bool _logging_on;
	bool _pause;
	bool _reset;
	bool _can_reset;
	bool _enable_grav_comp;

	bool _reset_complete;
};

}  // namespace Sai2Interfaces

#endif /* SAI2_INTERFACES_SIMVIZ_REDIS_INTERFACE_H */