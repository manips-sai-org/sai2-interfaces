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
FILE(COPY simviz_config_pendulum.xml simviz_config_panda.xml simviz_config_kukas.xml DESTINATION ${RESOURCE_DIR})
FILE(CREATE_LINK ${PROJECT_SOURCE_DIR}/examples/urdf ${RESOURCE_DIR}/urdf SYMBOLIC)