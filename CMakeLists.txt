cmake_minimum_required(VERSION 3.16)
project(AntMatrix C)

set(CMAKE_C_STANDARD 23)

# OMP support
# if(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
#     set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
#     set(OpenMP_C_LIB_NAMES "omp")
#     set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
# endif()
# find_package(OpenMP REQUIRED)

find_package(raylib REQUIRED)
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
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
