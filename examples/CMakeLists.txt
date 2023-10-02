set(EXAMPLE_NAME 01-simviz)
# create an executable
ADD_EXECUTABLE (${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

# and link the library against the executable
TARGET_LINK_LIBRARIES (${EXAMPLE_NAME}
	${SAI2-INTERFACES_LIBRARIES}
	)

# export resources such as model files.
SET(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
FILE(MAKE_DIRECTORY ${RESOURCE_DIR})
FILE(COPY simviz_config.xml world1.urdf world2.urdf world3.urdf rbot.urdf DESTINATION ${RESOURCE_DIR})