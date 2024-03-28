set(EXAMPLE_NAME 03-main-interface)
# create an executable
add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

# and link the library against the executable
target_link_libraries(${EXAMPLE_NAME} ${SAI2-INTERFACES_LIBRARIES})

# export resources such as model files.
set(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(MAKE_DIRECTORY ${RESOURCE_DIR})
file(COPY panda_controller_config.xml main_config.xml config_kukas.xml webui_template.html
     DESTINATION ${RESOURCE_DIR})
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/examples/urdf ${RESOURCE_DIR}/urdf
     SYMBOLIC)