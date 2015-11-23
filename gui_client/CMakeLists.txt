cmake_minimum_required(VERSION 2.8)
project(gui_client)
include_directories(../utils)
aux_source_directory(. SRC_LIST)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${UI_HEADERS} ${CODE_FILES})

target_link_libraries(${PROJECT_NAME} Qt5::Widgets utils)
