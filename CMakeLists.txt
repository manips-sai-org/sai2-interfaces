CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT (SAI2-INTERFACES)
SET (PROJECT_VERSION 0.1.0)

set(CMAKE_CXX_FLAGS "-std=c++17 -I/usr/include -I/usr/local/include -fPIC")
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/homebrew/include")
endif()

# set default build to release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release)
endif()

# include Eigen
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

# include Chai3d
find_package(CHAI3D REQUIRED)
include_directories(${CHAI3D_INCLUDE_DIRS})
add_definitions(${CHAI3D_DEFINITIONS})
link_directories(${CHAI3D_LIBRARY_DIRS})

# - SAI2-COMMON
FIND_PACKAGE(SAI2-COMMON REQUIRED)
include_directories(${SAI2-COMMON_INCLUDE_DIRS})

# - SAI2-URDF
FIND_PACKAGE(SAI2-URDF REQUIRED)
include_directories(${SAI2-URDF_INCLUDE_DIRS})

# - SAI2-MODEL
find_package(SAI2-MODEL REQUIRED)
include_directories(${SAI2-MODEL_INCLUDE_DIRS})

# - SAI2-SIMULATION
find_package(SAI2-SIMULATION REQUIRED)
include_directories(${SAI2-SIMULATION_INCLUDE_DIRS})
add_definitions(${SAI2-SIMULATION_DEFINITIONS})

# - SAI2-GRAPHICS
FIND_PACKAGE(SAI2-GRAPHICS REQUIRED)
include_directories(${SAI2-GRAPHICS_INCLUDE_DIRS})
add_definitions(${SAI2-GRAPHICS_DEFINITIONS})

# - SAI2-PRIMITIVES
FIND_PACKAGE(SAI2-PRIMITIVES REQUIRED)
include_directories(${SAI2-PRIMITIVES_INCLUDE_DIRS})

# add redis interface classes
SET(SAI2_INTERFACES_SOURCE
  ${PROJECT_SOURCE_DIR}/src/simviz/SimVizRedisInterface.cpp
  ${PROJECT_SOURCE_DIR}/src/simviz/SimVizConfigParser.cpp
  ${PROJECT_SOURCE_DIR}/src/controller/RobotControllerRedisInterface.cpp
  ${PROJECT_SOURCE_DIR}/src/controller/RobotControllerConfigParser.cpp
  ${PROJECT_SOURCE_DIR}/src/MainRedisInterface.cpp
)

SET(SAI2-INTERFACES_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES (${SAI2-INTERFACES_INCLUDE_DIRS})

# Create the library
ADD_LIBRARY (sai2-interfaces STATIC
	${SAI2_INTERFACES_SOURCE}
	)

SET(SAI2-INTERFACES_LIBRARIES
sai2-interfaces
  ${SAI2-PRIMITIVES_LIBRARIES}
	${SAI2-SIMULATION_LIBRARIES}
	${SAI2-GRAPHICS_LIBRARIES}
	${CHAI3D_LIBRARIES}
	${SAI2-MODEL_LIBRARIES}
	${SAI2-URDF_LIBRARIES}
	${SAI2-COMMON_LIBRARIES}
	)

ADD_SUBDIRECTORY(examples)

# SET (SAI2-INTERFACES_DEFINITIONS ${PROJECT_DEFINITIONS})

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
