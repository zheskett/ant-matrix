cmake_minimum_required(VERSION 3.16)
project(AntMatrix C)

set(CMAKE_C_STANDARD 11)

find_package(raylib REQUIRED)
file(GLOB SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")

add_executable(${PROJECT_NAME} ${SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)