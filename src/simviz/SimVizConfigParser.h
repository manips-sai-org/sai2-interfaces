#ifndef SAI2_INTERFACES_SIMVIZ_CONFIG_PARSER_H
#define SAI2_INTERFACES_SIMVIZ_CONFIG_PARSER_H

#include "SimVizConfig.h"

namespace Sai2Interfaces
{

class SimVizConfigParser
{
public:
    SimVizConfigParser() = default;
    ~SimVizConfigParser() = default;

	/**
	 * @brief Parses the config file and returns a SimVizConfig object.
	 * It will throw an error if the config cannot be parsed properly.
	 *
	 * @param config_file full path to the config file
	 * @return a SimVizConfig object
	 */
    SimVizConfig parseConfig(const std::string& config_file);
};

} // namespace Sai2Interfaces

#endif // SAI2_INTERFACES_SIMVIZ_CONFIG_PARSER_H