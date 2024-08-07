set( EXAMPLE_FOLDER_PATH ${CMAKE_CURRENT_SOURCE_DIR} )
add_definitions(-DEXAMPLE_FOLDER_PATH="${EXAMPLE_FOLDER_PATH}")

set(EXAMPLE_NAME MainRedisInterfaceExample)
add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)

# and link the library against the executable
target_link_libraries(${EXAMPLE_NAME} ${SAI2-INTERFACES_LIBRARIES})