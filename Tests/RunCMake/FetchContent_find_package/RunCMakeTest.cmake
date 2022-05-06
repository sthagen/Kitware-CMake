include(RunCMake)

unset(RunCMake_TEST_NO_CLEAN)

function(run_FetchContent_pkgRedirects)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CMAKE_FIND_PACKAGE_REDIRECTS_DIR-AlwaysEmptied-build)
  run_cmake(CMAKE_FIND_PACKAGE_REDIRECTS_DIR-AlwaysEmptied-Setup)
  set(RunCMake_TEST_NO_CLEAN 1)
  run_cmake(CMAKE_FIND_PACKAGE_REDIRECTS_DIR-AlwaysEmptied)
endfunction()

run_cmake(CMAKE_FIND_PACKAGE_REDIRECTS_DIR-Exists)
run_FetchContent_pkgRedirects()
run_cmake(BadArgs_find_package)
run_cmake(PreferFetchContent)
run_cmake(Prefer_find_package)
run_cmake(ProjectProvidesPackageConfigFiles)
run_cmake(Try_find_package-ALWAYS)
run_cmake(Try_find_package-NEVER)
run_cmake(Try_find_package-OPT_IN)
run_cmake(Try_find_package-BOGUS)
run_cmake(Redirect_find_package_MODULE)
