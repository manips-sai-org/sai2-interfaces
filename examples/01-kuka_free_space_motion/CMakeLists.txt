# create an executable
ADD_EXECUTABLE (01-kuka_free_space_motion 01-kuka_free_space_motion.cpp)

# and link the library against the executable
TARGET_LINK_LIBRARIES (01-kuka_free_space_motion
	${SAI2-INTERFACES_EXAMPLES_COMMON_LIBRARIES}
	)

# export resources such as model files.
# NOTE: this requires an install build
SET(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
FILE(MAKE_DIRECTORY ${RESOURCE_DIR})
FILE(COPY world.urdf kuka_iiwa.urdf DESTINATION ${RESOURCE_DIR})
