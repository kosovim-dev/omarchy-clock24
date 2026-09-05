cmake_minimum_required(VERSION 3.16)
project(OmarchyClock24Plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml)

qt_add_qml_module(clock24plugin
    URI Clock24
    VERSION 1.0
    SOURCES
        src/Clock24Item.h
        src/Clock24Item.cpp
        src/SunRise.h
        src/SunRise.cpp
)

target_include_directories(clock24plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(clock24plugin PRIVATE Qt6::Quick)