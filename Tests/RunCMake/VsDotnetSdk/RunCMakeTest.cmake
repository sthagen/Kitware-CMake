cmake_policy(SET CMP0053 NEW)
include(RunCMake)

run_cmake(VsDotnetSdkCustomCommandsSource)
run_cmake(VsDotnetSdkStartupObject)
run_cmake(VsDotnetSdkDefines)
run_cmake(DotnetSdkVariables)
run_cmake(VsDotnetSdkXamlFiles)

function(run_VsDotnetSdk)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/VsDotnetSdk-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(VsDotnetSdk)
  set(build_flags /restore)
  run_cmake_command(VsDotnetSdk-build ${CMAKE_COMMAND} --build . -- ${build_flags})
endfunction()
run_VsDotnetSdk()

function(runCmakeAndBuild CASE)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${CASE}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${CASE})
  run_cmake_command(${CASE}-build ${CMAKE_COMMAND} --build .)
endfunction()

runCmakeAndBuild(VsDotnetSdkCustomCommandsTarget)
runCmakeAndBuild(VsDotnetSdkNugetRestore)
