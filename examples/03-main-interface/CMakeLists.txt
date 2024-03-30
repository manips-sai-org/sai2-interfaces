set(EXAMPLE_NAME 03-main-interface)
# create an executable
add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

# and link the library against the executable
target_link_libraries(${EXAMPLE_NAME} ${SAI2-INTERFACES_LIBRARIES})