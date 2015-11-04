cmake_minimum_required(VERSION 2.8)
#add_subdirectory(server)
#add_subdirectory(slave)
#add_subdirectory(client)



project(serial)
aux_source_directory(. SRC_LIST)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} boost_serialization spread dl m rt nsl)
