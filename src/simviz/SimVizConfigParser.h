#ifndef SIMVIZ_CONFIG_PARSER_H__
#define SIMVIZ_CONFIG_PARSER_H__

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

#endif // SIMVIZ_CONFIG_PARSER_H__