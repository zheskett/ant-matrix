cmake_minimum_required(VERSION 3.16)
project(AntMatrix C)

option(USE_WEB_RAYLIB "Build for WASM with an externally-built raylib-web" OFF)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
list(FILTER SRC EXCLUDE REGEX ".*/src/main\\.c$")

if(USE_WEB_RAYLIB)
    set(CMAKE_BUILD_TYPE Release)
    set(RAYLIB_WEB_ROOT "$ENV{RAYLIB_WEB_ROOT}" CACHE PATH "Folder that contains libraylib.a built with PLATFORM=Web")

    if(NOT RAYLIB_WEB_ROOT)
        message(FATAL_ERROR "Set RAYLIB_WEB_ROOT when USE_WEB_RAYLIB=ON")
    endif()

    add_library(raylib STATIC IMPORTED)
    set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${RAYLIB_WEB_ROOT}/lib/libraylib.a"
        INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_WEB_ROOT}/include"
    )
else()
    find_package(raylib REQUIRED)
endif()

add_library(AntMatrix_static STATIC ${SRC})
target_include_directories(AntMatrix_static PUBLIC
    "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/lib")

target_link_libraries(AntMatrix_static PUBLIC raylib)

add_executable(AntMatrix src/main.c)
target_link_libraries(AntMatrix PRIVATE AntMatrix_static)

# Extras only for WASM
if(USE_WEB_RAYLIB)
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # CMake will generate AntMatrix.html
    set(WASM_SHELL "${CMAKE_SOURCE_DIR}/web/wasm-shell.html")
    configure_file(${CMAKE_SOURCE_DIR}/web/favicon.png ${CMAKE_CURRENT_BINARY_DIR}/favicon.png COPYONLY)

    # Preload the assets folder so FS.open("assets/…") works:
    target_link_options(AntMatrix PRIVATE
        "-sUSE_GLFW=3"
        "-sASYNCIFY"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sUSE_WEBGL2=1"
        "-sOFFSCREENCANVAS_SUPPORT=1"
        "--shell-file=${WASM_SHELL}"
        "--preload-file=${CMAKE_SOURCE_DIR}/assets@assets")
endif()

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
add_custom_target(Assets ALL COMMENT "Copying assets folder")
add_dependencies(Assets ${PROJECT_NAME})

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()