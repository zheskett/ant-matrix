cmake_minimum_required(VERSION 3.16)
project(AntMatrix C)

set(CMAKE_C_STANDARD 11)

find_package(raylib REQUIRED)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/lib/*.c")

add_executable(${PROJECT_NAME} ${SRC})

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
add_custom_target(Assets ALL COMMENT "Copying assets folder")
add_dependencies(Assets ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)