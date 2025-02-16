cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(showimage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(showimage ${CMAKE_CURRENT_SOURCE_DIR}/src/ShowImage.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/splash.cxx)

target_link_libraries(showimage PUBLIC Qt6::Widgets)

install(TARGETS showimage DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
