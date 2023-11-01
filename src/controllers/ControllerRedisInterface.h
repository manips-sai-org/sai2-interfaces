#ifndef SAI2_INTERFACES_CONTROLLER_REDIS_INTERFACE_H
#define SAI2_INTERFACES_CONTROLLER_REDIS_INTERFACE_H

#include <string>
#include "Sai2Primitives.h"
#include "Sai2Model.h"

class ControllerRedisInterface
{
public:
    ControllerRedisInterface(const std::string& config_file);
    ~ControllerRedisInterface() = default;

    void run();

private:

    void reset();
    void initializeRedisDatabase();

    void getParametrization();
    void processParametrization();

    std::string _config_file;

    std::map<std::string, Sai2Model::Sai2Model> _robot_models;
    std::map<std::string, Sai2Primitives::RobotController> _robot_controllers;
    std::map<std::string, bool> _reset;

    Sai2Common::RedisClient _redis_client;

    std::map<std::string, Eigen::VectorXd> _robot_control_torques;
    std::map<std::string, Eigen::VectorXd> _robot_q;
    std::map<std::string, Eigen::VectorXd> _robot_dq;
    std::map<std::string, Eigen::Matrix4d> _object_pose;
    std::map<std::string, Eigen::Vector6d> _object_vel;

    std::map<std::string, Eigen::Vector3d> _sensed_force;
    std::map<std::string, Eigen::Vector3d> _sensed_moment;

    bool _is_sim_paused;
};

#endif // SAI2_INTERFACES_CONTROLLER_REDIS_INTERFACE_H