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
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt5_wrap_ui(UI_HEADERS mainwindow.ui file_entry.ui file_dialog.ui)
qt5_add_resources(RSCS resources.qrc)
add_custom_target(gen_qrc DEPENDS ${RSCS})

add_executable(${PROJECT_NAME} ${SRC_LIST} ${UI_HEADERS} ${CODE_FILES} ${RSCS})
add_dependencies(${PROJECT_NAME} gen_qrc)

target_link_libraries(${PROJECT_NAME} Qt5::Widgets utils)
