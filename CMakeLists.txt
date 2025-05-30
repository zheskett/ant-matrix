cmake_minimum_required(VERSION 3.16)
project(AntMatrix C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c" "${CMAKE_SOURCE_DIR}/lib/*.c")
list(FILTER SRC EXCLUDE REGEX ".*/src/main\\.c$")

# Create static library for test purposes
add_library(${PROJECT_NAME}_static STATIC ${SRC})
target_include_directories(${PROJECT_NAME}_static PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/lib")
target_link_libraries(${PROJECT_NAME}_static PUBLIC raylib m)

add_executable(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/src/main.c")
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_static)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
add_custom_target(Assets ALL COMMENT "Copying assets folder")
add_dependencies(Assets ${PROJECT_NAME})

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()
