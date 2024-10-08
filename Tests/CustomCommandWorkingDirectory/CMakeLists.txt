cmake_minimum_required(VERSION 3.10)
project(TestWorkingDir)

add_custom_command(
  OUTPUT "${TestWorkingDir_BINARY_DIR}/working.c"
  COMMAND "${CMAKE_COMMAND}" -E copy ./working.c.in "${TestWorkingDir_BINARY_DIR}/working.c"
  WORKING_DIRECTORY "${TestWorkingDir_SOURCE_DIR}"
  COMMENT "custom command"
)

set_source_files_properties(
  "${TestWorkingDir_BINARY_DIR}/customTarget1.c"
  "${TestWorkingDir_BINARY_DIR}/customTarget2.c"
  PROPERTIES GENERATED 1)

add_executable(working "${TestWorkingDir_BINARY_DIR}/working.c"
  "${TestWorkingDir_BINARY_DIR}/customTarget1.c")

add_custom_target(
  Custom ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different ./customTarget.c "${TestWorkingDir_BINARY_DIR}/customTarget1.c"
  BYPRODUCTS "${TestWorkingDir_BINARY_DIR}/customTarget1.c"
  WORKING_DIRECTORY  "${TestWorkingDir_SOURCE_DIR}"
)

add_dependencies(working Custom)

file(MAKE_DIRECTORY ${TestWorkingDir_BINARY_DIR}/work)
add_custom_command(
  OUTPUT working2.c # Relative to build tree
  COMMAND "${CMAKE_COMMAND}" -E copy ${TestWorkingDir_SOURCE_DIR}/working.c.in ../working2.c
  DEPENDS ${TestWorkingDir_SOURCE_DIR}/working.c.in/ # trailing slash should be removed
  WORKING_DIRECTORY work/ # Relative to build tree, trailing slash
  )
add_executable(working2 working2.c ${TestWorkingDir_BINARY_DIR}/customTarget2.c)

add_custom_target(
  Custom2 ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${TestWorkingDir_SOURCE_DIR}/customTarget.c ../customTarget2.c
  BYPRODUCTS "${TestWorkingDir_BINARY_DIR}/customTarget2.c"
  WORKING_DIRECTORY work/ # Relative to build tree, trailing slash
)

add_dependencies(working2 Custom2)

file(MAKE_DIRECTORY ${TestWorkingDir_BINARY_DIR}/genex)
add_custom_command(
  OUTPUT "${TestWorkingDir_BINARY_DIR}/genex/working.c"
  COMMAND "${CMAKE_COMMAND}" -E copy "${TestWorkingDir_SOURCE_DIR}/working.c.in" "${TestWorkingDir_BINARY_DIR}/genex/working.c"
  WORKING_DIRECTORY "$<0:not_used/>${TestWorkingDir_BINARY_DIR}/$<1:genex>/"
  COMMENT "custom command"
)

add_executable(workinggenex "${TestWorkingDir_BINARY_DIR}/genex/working.c"
  "${TestWorkingDir_BINARY_DIR}/genex/customTarget.c")

add_custom_target(
  CustomGenex ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TestWorkingDir_SOURCE_DIR}/customTarget.c" "${TestWorkingDir_BINARY_DIR}/genex/customTarget.c"
  BYPRODUCTS "${TestWorkingDir_BINARY_DIR}/genex/customTarget.c"
  WORKING_DIRECTORY "$<0:not_used/>${TestWorkingDir_BINARY_DIR}/$<1:genex>/"
)

add_dependencies(workinggenex CustomGenex)
