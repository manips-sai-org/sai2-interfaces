#ifndef SIMVIZ_REDIS_INTERFACE_H_
#define SIMVIZ_REDIS_INTERFACE_H_

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <Eigen/Dense>

#include "redis/RedisClient.h"
#include "Sai2Graphics.h"
#include "Sai2Simulation.h"
#include "timer/LoopTimer.h"

namespace Sai2Interfaces
{


class SimVizRedisInterface
{
public:
    SimVizRedisInterface(const std::string& world_file);
    ~SimVizRedisInterface() = default;

    void run();

private:

    void reset();
    void initializeRedisDatabase();

    void vizLoopRun();
    void simLoopRun();
    void getSimParametrization();
    void processSimParametrization();
    void readInputs();
    void writeOutputs();

    std::string _current_world_file;

    Sai2Common::RedisClient _redis_client;

    std::unique_ptr<Sai2Graphics::Sai2Graphics> _graphics;
    std::unique_ptr<Sai2Simulation::Sai2Simulation> _simulation;

    std::map<std::string, Eigen::VectorXd> _ui_torques;

    std::map<std::string, Eigen::VectorXd> _robot_control_torques;
    std::map<std::string, Eigen::VectorXd> _robot_q;
    std::map<std::string, Eigen::VectorXd> _robot_dq;
    std::map<std::string, Eigen::Matrix4d> _object_pose;
    std::map<std::string, Eigen::VectorXd> _object_vel;

    std::mutex _mutex_parametrization;
    std::mutex _mutex_torques;

    bool _pause;
    bool _reset;
    bool _enable_grav_comp;
    std::string _redis_world_file;

};

} // namespace Sai2Interfaces

#endif /* SIMVIZ_REDIS_INTERFACE_H_ */