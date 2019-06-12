CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

PROJECT (SAI2-INTERFACES NONE)
SET (PROJECT_VERSION 0.1.0)

SET (SAI2-INTERFACES_UI_DIR
    ${PROJECT_SOURCE_DIR}/interface
)

# 
# export package
# 

# export package for use from the build tree
EXPORT (PACKAGE SAI2-INTERFACES)

# configure package
# message(Project definitions : \n ${SAI2-INTERFACES_UI_DIR} \n\n)

# create package configuration and version files
CONFIGURE_FILE(SAI2-INTERFACESConfig.cmake.in "${PROJECT_BINARY_DIR}/SAI2-INTERFACESConfig.cmake" @ONLY)
INCLUDE(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SAI2-INTERFACESConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
