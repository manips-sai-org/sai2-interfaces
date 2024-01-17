#ifndef SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_PARSER_H
#define SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_PARSER_H

#include <tinyxml2.h>

#include "RobotControllerConfig.h"

namespace Sai2Interfaces {

class RobotControllerConfigParser {
public:
	RobotControllerConfigParser() = default;
	~RobotControllerConfigParser() = default;

	RobotControllerConfig parseConfig(const std::string& config_file);

private:
	std::vector<std::variant<JointTaskConfig, MotionForceTaskConfig>>
	parseSingleControllerConfig(tinyxml2::XMLElement* xml);

	JointTaskConfig parseJointTaskConfig(tinyxml2::XMLElement* xml);
	MotionForceTaskConfig parseMotionForceTaskConfig(tinyxml2::XMLElement* xml);

	std::string _config_file_name;
};

}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_ROBOT_CONTROLLER_CONFIG_PARSER_H