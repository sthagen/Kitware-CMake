cmake_minimum_required(VERSION 3.10)
project(OutDir C)

get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
  foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" CONFIG)
    list(APPEND configs "${CONFIG}")
  endforeach()
  unset(CMAKE_BUILD_TYPE CACHE)
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" configs)
endif()

set(top "${OutDir_BINARY_DIR}")
foreach(config ${configs})
  foreach(type archive runtime library)
    string(TOUPPER "${type}" TYPE)
    set(CMAKE_${TYPE}_OUTPUT_DIRECTORY_${config} "${top}/${type}/$<CONFIG>")
    file(REMOVE_RECURSE "${top}/${type}")
  endforeach()
endforeach()

add_subdirectory(../COnly COnly)

add_custom_command(
  OUTPUT OutDir.h
  COMMAND ${CMAKE_COMMAND} -Dtop=${top} -Dcfg_dir=$<CONFIG> -P ${OutDir_SOURCE_DIR}/OutDir.cmake
  DEPENDS COnly ${OutDir_SOURCE_DIR}/OutDir.cmake
  )
include_directories(${top})
add_executable(OutDir OutDir.c OutDir.h)
