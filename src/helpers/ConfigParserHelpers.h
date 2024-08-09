#ifndef SAI2_INTERFACES_CONFIG_PARSER_HELPERS_H
#define SAI2_INTERFACES_CONFIG_PARSER_HELPERS_H

#include <tinyxml2.h>

#include <Eigen/Dense>
#include <string>
#include <vector>

// \cond
// for internal use only
namespace Sai2Interfaces {
namespace ConfigParserHelpers {
Eigen::Quaterniond rpyToQuaternion(double roll, double pitch, double yaw);
std::vector<std::string> splitString(const std::string& str,
									 const std::vector<char>& separators = {
										 ' ', '\t', '\n', ',', '[', ']'});
Eigen::Vector3d parseVector3d(const char* vec_str);
Eigen::Vector3d parseVector3d(tinyxml2::XMLElement* xml,
							  std::string attribute_name = "xyz");
Eigen::Affine3d parsePose(tinyxml2::XMLElement* xml);

}  // namespace ConfigParserHelpers
}  // namespace Sai2Interfaces

// \endcond

#endif	// SAI2_INTERFACES_CONFIG_PARSER_HELPERS_H