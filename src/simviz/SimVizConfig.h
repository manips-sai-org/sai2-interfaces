#ifndef SAI2_INTERFACES_SIMVIZ_CONFIG_H
#define SAI2_INTERFACES_SIMVIZ_CONFIG_H

#include <string>
#include <Eigen/Dense>

namespace Sai2Interfaces {

struct SimForceSensorConfig
{
    std::string robot_name = "";
    std::string link_name = "";
    Eigen::Affine3d transform_in_link = Eigen::Affine3d::Identity();
    double cutoff_frequency = 0.0;
};

struct SimLoggerConfig {
	std::string folder_name = "logs_simviz";
	double frequency = 100.0;
	bool start_with_logger_on = false;
	bool add_timestamp_to_filename = true;
};

struct SimVizConfig
{
    std::string world_file = "";
    bool enable_joint_limits = true;
    double friction_coefficient = 0.0;
    double collision_restitution = 0.0;
    double timestep = 0.001;

    std::vector<SimForceSensorConfig> force_sensors = {};

	SimLoggerConfig logger_config;
};

} // namespace Sai2Interfaces

#endif // SAI2_INTERFACES_SIMVIZ_CONFIG_H
