cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(showimage)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (MSVC)
    add_compile_options(/W4 /WX)
else(MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif (MSVC)

find_package(Qt6 COMPONENTS Widgets Concurrent REQUIRED)

add_executable(showimage ${CMAKE_CURRENT_SOURCE_DIR}/src/ShowImage.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/enlighten.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/files.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/histogram.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/scale.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/slice.cxx
                         ${CMAKE_CURRENT_SOURCE_DIR}/src/splash.cxx)

target_link_libraries(showimage PUBLIC Qt6::Widgets Qt6::Concurrent)

if (WIN32)
    set_target_properties(showimage PROPERTIES WIN32_EXECUTABLE TRUE)
endif (WIN32)

install(TARGETS showimage DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
