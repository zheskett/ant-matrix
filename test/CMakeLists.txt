enable_testing()
set(TEST_COMMON_LIBS AntMatrix_static)

file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

foreach(test_src ${TEST_SRC})
    get_filename_component(test_name "${test_src}" NAME_WE)
    add_executable(${test_name} "${test_src}")
    target_include_directories(${test_name} PRIVATE ${TEST_COMMON_INCS})
    target_link_libraries(${test_name} PRIVATE ${TEST_COMMON_LIBS})

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()