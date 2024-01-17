set(EXAMPLE_NAME 02-simviz-and-controller)
# create an executable
add_executable(02-control 02-control.cpp)
add_executable(02-simviz 02-simviz.cpp)

# and link the library against the executable
target_link_libraries(02-control ${SAI2-INTERFACES_LIBRARIES})
target_link_libraries(02-simviz ${SAI2-INTERFACES_LIBRARIES})

# export resources such as model files.
set(RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resources)
file(MAKE_DIRECTORY ${RESOURCE_DIR})
file(COPY panda_controller_config.xml simviz_config_panda.xml
     DESTINATION ${RESOURCE_DIR})
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/examples/urdf ${RESOURCE_DIR}/urdf
     SYMBOLIC)
