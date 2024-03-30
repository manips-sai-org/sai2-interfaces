set(EXAMPLE_NAME 02-simviz-and-controller)
# create an executable
add_executable(02-control 02-control.cpp)
add_executable(02-simviz 02-simviz.cpp)

# and link the library against the executable
target_link_libraries(02-control ${SAI2-INTERFACES_LIBRARIES})
target_link_libraries(02-simviz ${SAI2-INTERFACES_LIBRARIES})