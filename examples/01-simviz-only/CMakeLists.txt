set(EXAMPLE_NAME 01-simviz)
# create an executable
add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

# and link the library against the executable
target_link_libraries(${EXAMPLE_NAME} ${SAI2-INTERFACES_LIBRARIES})

# export resources such as model files.
set(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(MAKE_DIRECTORY ${RESOURCE_DIR})
file(COPY simviz_config_pendulum.xml simviz_config_panda.xml
          simviz_config_kukas.xml DESTINATION ${RESOURCE_DIR})
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/examples/urdf ${RESOURCE_DIR}/urdf
     SYMBOLIC)
