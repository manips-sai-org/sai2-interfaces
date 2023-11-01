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

    SimVizConfig parseConfig(const std::string& config_file);
};

} // namespace Sai2Interfaces

#endif // SAI2_INTERFACES_SIMVIZ_CONFIG_PARSER_H