include(RunCMake)

# Test experimental gate
run_cmake(ExperimentalGate)
run_cmake(ExperimentalWarning)

# Enable experimental feature and suppress warnings
set(RunCMake_TEST_OPTIONS
  -Wno-dev
  "-DCMAKE_EXPERIMENTAL_EXPORT_PACKAGE_INFO:STRING=b80be207-778e-46ba-8080-b23bba22639e"
  )

function(run_cmake_install test)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${test}-build)
  if (NOT RunCMake_GENERATOR_IS_MULTI_CONFIG)
    list(APPEND RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=Release)
  endif()

  run_cmake(${test})

  set(RunCMake_TEST_NO_CLEAN TRUE)
  run_cmake_command(${test}-build ${CMAKE_COMMAND} --build . --config Release)
  run_cmake_command(${test}-install ${CMAKE_COMMAND} --install . --config Release)
endfunction()

# Test incorrect usage
run_cmake(BadArgs1)
run_cmake(BadArgs2)
run_cmake(BadName)
run_cmake(BadDefaultTarget)
run_cmake(ReferencesNonExportedTarget)
run_cmake(ReferencesWronglyExportedTarget)
run_cmake(ReferencesWronglyImportedTarget)
run_cmake(ReferencesWronglyNamespacedTarget)
run_cmake(DependsMultipleDifferentNamespace)
run_cmake(DependsMultipleDifferentSets)

# Test functionality
run_cmake(Appendix)
run_cmake(InterfaceProperties)
run_cmake(Metadata)
run_cmake(ProjectMetadata)
run_cmake(NoProjectMetadata)
run_cmake(Minimal)
run_cmake(MinimalVersion)
run_cmake(LowerCaseFile)
run_cmake(Requirements)
run_cmake(TargetTypes)
run_cmake(DependsMultiple)
run_cmake(DependsMultipleNotInstalled)
run_cmake(Config)
run_cmake(EmptyConfig)
run_cmake(FileSetHeaders)
run_cmake_install(Destination)
