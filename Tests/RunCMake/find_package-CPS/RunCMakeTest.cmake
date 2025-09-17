include(RunCMake)

run_cmake(ExperimentalWarning)

# Enable experimental feature and suppress warnings
set(RunCMake_TEST_OPTIONS
  -Wno-dev
  "-DCMAKE_EXPERIMENTAL_FIND_CPS_PACKAGES:STRING=e82e467b-f997-4464-8ace-b00808fff261"
  )

function(run_cmake_build test)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${test}-build)
  if(${ARGC} EQUAL 2)
    set(_build_type ${ARGV1})
  else()
    set(_build_type Release)
  endif()

  if(RunCMake_GENERATOR_IS_MULTI_CONFIG)
    list(APPEND RunCMake_TEST_OPTIONS -DCMAKE_CONFIGURATION_TYPES=${_build_type})
  else()
    list(APPEND RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=${_build_type})
  endif()

  run_cmake(${test})

  set(RunCMake_TEST_NO_CLEAN TRUE)
  run_cmake_command(${test}-build ${CMAKE_COMMAND} --build . --config ${_build_type})
endfunction()


# Version-matching tests
run_cmake(ExactVersion)
run_cmake(CompatVersion)
run_cmake(MultipleVersions)
run_cmake(VersionLimit1)
run_cmake(VersionLimit2)
run_cmake(TransitiveVersion)
run_cmake(CustomVersion)

# Metadata Tests
run_cmake(License)

# Version-matching failure tests
run_cmake(MissingVersion1)
run_cmake(MissingVersion2)
run_cmake(VersionLimit3)
run_cmake(VersionLimit4)

# Component-related failure tests
run_cmake(MissingTransitiveDependency)
run_cmake(MissingComponent)
run_cmake(MissingComponentDependency)
run_cmake(MissingTransitiveComponentCPS)
run_cmake(MissingTransitiveComponentCMake)
run_cmake(MissingTransitiveComponentDependency)

# Configuration selection tests
run_cmake_build(ConfigDefault)
run_cmake_build(ConfigFirst)
run_cmake_build(ConfigMapped)
run_cmake_build(ConfigMatchBuildType Test)
