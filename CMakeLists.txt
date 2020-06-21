cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_LIST_DIR})
  if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting CMAKE_BUILD_TYPE type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
  endif()
endif()

project(cop VERSION 0.1.0 LANGUAGES C)

set(COP_PUBLIC_INCLUDES cop_alloc.h cop_attributes.h cop_conversions.h cop_filemap.h cop_log.h cop_sort.h cop_thread.h cop_vec.h)

add_library(cop STATIC libcop/cop_filemap.c libcop/cop_alloc.c ${COP_PUBLIC_INCLUDES})
set_property(TARGET cop APPEND PROPERTY PUBLIC_HEADER ${COP_PUBLIC_INCLUDES})
set_property(TARGET cop PROPERTY ARCHIVE_OUTPUT_DIRECTORY "$<$<NOT:$<CONFIG:Release>>:$<CONFIG>>")

if ("x${CMAKE_C_COMPILER_ID}" STREQUAL "xMSVC")
  set_property(TARGET cop APPEND_STRING PROPERTY COMPILE_FLAGS " /W3")
else()
  set_property(TARGET cop APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

target_include_directories(cop PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>")

if (UNIX)
  target_link_libraries(cop pthread)
endif()

add_subdirectory(app_cop_vec_test)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake_support/copConfig.cmake.in ${CMAKE_BINARY_DIR}/cmake/cop/copConfig.cmake
  INSTALL_DESTINATION "lib/cmake/cop"
  NO_CHECK_REQUIRED_COMPONENTS_MACRO)
write_basic_package_version_file(${CMAKE_BINARY_DIR}/cmake/cop/copConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)

export(TARGETS cop FILE "${PROJECT_BINARY_DIR}/cmake/cop/copTargets.cmake")

install(TARGETS cop
  EXPORT cop_targets
  RUNTIME DESTINATION "bin$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  ARCHIVE DESTINATION "lib$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  LIBRARY DESTINATION "lib$<$<NOT:$<CONFIG:Release>>:/$<CONFIG>>"
  PUBLIC_HEADER DESTINATION include/cop
  INCLUDES DESTINATION include)
install(EXPORT cop_targets DESTINATION lib/cmake/cop FILE "copTargets.cmake")
install(FILES "${CMAKE_BINARY_DIR}/cmake/cop/copConfig.cmake"
              "${CMAKE_BINARY_DIR}/cmake/cop/copConfigVersion.cmake"
        DESTINATION "lib/cmake/cop")
install(FILES LICENSE DESTINATION share/doc/cop)
install(FILES README.md DESTINATION share/doc/cop)

