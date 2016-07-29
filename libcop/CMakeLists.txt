cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
  endif()
  project(cop VERSION 0.1.0 LANGUAGES C)
endif()

add_library(cop STATIC cop_filemap.c ../cop_alloc.h ../cop_attributes.h ../cop_conversions.h ../cop_filemap.h ../cop_thread.h ../cop_vec.h)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set_property(TARGET cop APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET cop APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_include_directories(cop PRIVATE "../..")
