CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

PROJECT (SAI2-INTERFACES)
SET (PROJECT_VERSION 0.1.0)

SET(CMAKE_CXX_FLAGS "-std=c++17 -I/usr/include -I/usr/local/include")
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  MESSAGE(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  # Set the default build, and also the possible values of build type for cmake-gui
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF()

SET (SAI2-INTERFACES_UI_DIR
    ${PROJECT_SOURCE_DIR}/interface
)

# include Eigen
find_package(Eigen3 QUIET)
include_directories(${EIGEN3_INCLUDE_DIR})

# include Chai3d 
# -- this is required by Chai3d to pick the correct path for OpenGL headers
FIND_PACKAGE(CHAI3D REQUIRED)
ADD_DEFINITIONS(${CHAI3D_DEFINITIONS})
if (CMAKE_SYSTEM_NAME MATCHES Darwin) 
  ADD_DEFINITIONS(-D_MACOSX -DMACOSX)
elseif (CMAKE_SYSTEM_NAME MATCHES Linux)
  ADD_DEFINITIONS(-D_LINUX -DLINUX)
endif () #NOTE: Windows is currently unsupported
LINK_DIRECTORIES(${CHAI3D_LIBRARY_DIRS})
INCLUDE_DIRECTORIES(${CHAI3D_INCLUDE_DIRS})

# - SAI2-MODEL
find_package(SAI2-MODEL REQUIRED)
include_directories(${SAI2-MODEL_INCLUDE_DIRS})

# - SAI2-SIMULATION
find_package(SAI2-SIMULATION REQUIRED)
include_directories(${SAI2-SIMULATION_INCLUDE_DIRS})

# - SAI2-GRAPHICS
FIND_PACKAGE(SAI2-GRAPHICS REQUIRED)
include_directories(${SAI2-GRAPHICS_INCLUDE_DIRS})

# - SAI2-PRIMITIVES
FIND_PACKAGE(SAI2-PRIMITIVES REQUIRED)
include_directories(${SAI2-PRIMITIVES_INCLUDE_DIRS})

# - SAI2-COMMON
FIND_PACKAGE(SAI2-COMMON REQUIRED)
include_directories(${SAI2-COMMON_INCLUDE_DIRS})

# - SAI2-URDF
FIND_PACKAGE(SAI2-URDF REQUIRED)
include_directories(${SAI2-URDF_INCLUDE_DIRS})

# - glfw3
find_package(glfw3 QUIET)
find_library(GLFW_LIBRARY glfw)


# add redis interface classes
SET(SAI2_INTERFACES_SOURCE
  ${PROJECT_SOURCE_DIR}/src/simviz/SimVizRedisInterface.cpp
  ${PROJECT_SOURCE_DIR}/src/simviz/SimVizConfigParser.cpp
)

SET(SAI2-INTERFACES_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES (${SAI2-INTERFACES_INCLUDE_DIRS})

# Create the library
ADD_LIBRARY (sai2-interfaces STATIC
	${SAI2_INTERFACES_SOURCE}
	)

SET(SAI2-INTERFACES_LIBRARIES
	sai2-interfaces
	${SAI2-MODEL_LIBRARIES}
	${SAI2-GRAPHICS_LIBRARIES}
	${SAI2-SIMULATION_LIBRARIES}
	${SAI2-PRIMITIVES_LIBRARIES}
	${SAI2-COMMON_LIBRARIES}
	${SAI2-URDF_LIBRARIES}
	${CHAI3D_LIBRARIES}
	${GLFW_LIBRARY}
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
