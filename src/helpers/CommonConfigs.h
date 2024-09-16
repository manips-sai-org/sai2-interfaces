#ifndef SAI2_INTERFACES_CONFIG_COMMON_CONFIGS_H
#define SAI2_INTERFACES_CONFIG_COMMON_CONFIGS_H

#include <string>

namespace Sai2Interfaces {
/**
 * @brief Configuration struct for the logger object (common to simviz and
 * controller configs)
 *
 * This is parsed from the xml file from the following element:
 * 	<logger folderName="..." logFrequency="..." enabledAtStartup="..."
 * addTimestampToFilename="..." />
 *
 */
struct LoggerConfig {
	/// @brief The path to the folder where the log files will be saved
	std::string folder_name = "log_files";
	/// @brief The frequency at which the logger will log the data
	double frequency = 100.0;
	/// @brief Whether to start the logger when the simulation starts
	bool start_with_logger_on = false;
	/// @brief Whether to add a timestamp to the filename of the log files
	bool add_timestamp_to_filename = true;

	bool operator==(const LoggerConfig& other) const {
		return (folder_name == other.folder_name) &&
			   (frequency == other.frequency) &&
			   (start_with_logger_on == other.start_with_logger_on) &&
			   (add_timestamp_to_filename == other.add_timestamp_to_filename);
	}

	LoggerConfig(const std::string& folder_name) : folder_name(folder_name) {}
};
}  // namespace Sai2Interfaces

#endif	// SAI2_INTERFACES_CONFIG_COMMON_CONFIGS_H