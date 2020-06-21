cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(app_cop_vec_test LANGUAGES C)

add_executable(cop_vec_test cop_vec_test.c)

if ("x${CMAKE_C_COMPILER_ID}" STREQUAL "xMSVC")
  set_property(TARGET cop_vec_test APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET cop_vec_test APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_include_directories(cop_vec_test PRIVATE "../..")
target_link_libraries(cop_vec_test cop)
