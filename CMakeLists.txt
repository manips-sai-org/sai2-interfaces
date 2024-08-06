cmake_minimum_required(VERSION 3.10)

project(SAI2-INTERFACES)
set(PROJECT_VERSION 0.1.0)

option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_FLAGS "-std=c++20 -I/usr/include -I/usr/local/include -fPIC")
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

# * SAI2-COMMON
find_package(SAI2-COMMON REQUIRED)
include_directories(${SAI2-COMMON_INCLUDE_DIRS})

# * SAI2-URDF
find_package(SAI2-URDF REQUIRED)
include_directories(${SAI2-URDF_INCLUDE_DIRS})

# * SAI2-MODEL
find_package(SAI2-MODEL REQUIRED)
include_directories(${SAI2-MODEL_INCLUDE_DIRS})

# * SAI2-SIMULATION
find_package(SAI2-SIMULATION REQUIRED)
include_directories(${SAI2-SIMULATION_INCLUDE_DIRS})
add_definitions(${SAI2-SIMULATION_DEFINITIONS})

# * SAI2-GRAPHICS
find_package(SAI2-GRAPHICS REQUIRED)
include_directories(${SAI2-GRAPHICS_INCLUDE_DIRS})
add_definitions(${SAI2-GRAPHICS_DEFINITIONS})

# * SAI2-PRIMITIVES
find_package(SAI2-PRIMITIVES REQUIRED)
include_directories(${SAI2-PRIMITIVES_INCLUDE_DIRS})

# add redis interface classes
set(SAI2_INTERFACES_SOURCE
    ${PROJECT_SOURCE_DIR}/src/simviz/SimVizRedisInterface.cpp
    ${PROJECT_SOURCE_DIR}/src/simviz/SimVizConfigParser.cpp
    ${PROJECT_SOURCE_DIR}/src/controller/RobotControllerRedisInterface.cpp
    ${PROJECT_SOURCE_DIR}/src/controller/RobotControllerConfigParser.cpp
    ${PROJECT_SOURCE_DIR}/src/MainRedisInterface.cpp)

set(SAI2-INTERFACES_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/third_party/glaze/include)
include_directories(${SAI2-INTERFACES_INCLUDE_DIRS})

set(SAI2-INTERFACES_UI_DIR ${PROJECT_SOURCE_DIR}/ui)
add_definitions(-DUI_FOLDER="${SAI2-INTERFACES_UI_DIR}")

# Create the library
add_library(sai2-interfaces STATIC ${SAI2_INTERFACES_SOURCE})

# Ensure Glaze include directories are part of the target
target_include_directories(
  sai2-interfaces
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/third_party/glaze/include>
         $<INSTALL_INTERFACE:include>)

set(SAI2-INTERFACES_LIBRARIES
    sai2-interfaces
    ${SAI2-PRIMITIVES_LIBRARIES}
    ${SAI2-SIMULATION_LIBRARIES}
    ${SAI2-GRAPHICS_LIBRARIES}
    ${CHAI3D_LIBRARIES}
    ${SAI2-MODEL_LIBRARIES}
    ${SAI2-URDF_LIBRARIES}
    ${SAI2-COMMON_LIBRARIES})

# add examples
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

#
# export package
#
set(SAI2-INTERFACES_UI_DIR ${PROJECT_SOURCE_DIR}/interface)

# export package for use from the build tree
export(TARGETS sai2-interfaces
       FILE ${PROJECT_BINARY_DIR}/SAI2-INTERFACESTargets.cmake)
export(PACKAGE SAI2-INTERFACES)

# configure package
set(CONF_INCLUDE_DIRS ${SAI2-INTERFACES_INCLUDE_DIRS})
set(CONF_LIBRARIES sai2-interfaces)

# create package configuration and version files
configure_file(SAI2-INTERFACESConfig.cmake.in
               "${PROJECT_BINARY_DIR}/SAI2-INTERFACESConfig.cmake" @ONLY)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SAI2-INTERFACESConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
