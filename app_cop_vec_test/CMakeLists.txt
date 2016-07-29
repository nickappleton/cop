cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

project(app_cop_vec_test)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
endif()

add_executable(cop_vec_test cop_vec_test.c)
target_include_directories(cop_vec_test PRIVATE "../..")
target_link_libraries(cop_vec_test cop)

add_subdirectory("../../cop" "${CMAKE_CURRENT_BINARY_DIR}/cop_dep")
