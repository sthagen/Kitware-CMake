include(RunCMake)

set(RunCMake_TEST_OPTIONS
  -DCMAKE_MINIMUM_REQUIRED_VERSION:STATIC=
  # Simulate a previous CMake run that used `project(... VERSION ...)`
  # in a non-injected call site.
  -DCMAKE_PROJECT_VERSION:STATIC=1.2.3
  -DCMAKE_PROJECT_VERSION_MAJOR:STATIC=1
  -DCMAKE_PROJECT_VERSION_MINOR:STATIC=2
  -DCMAKE_PROJECT_VERSION_PATCH:STATIC=3
  )
run_cmake(Inject)
unset(RunCMake_TEST_OPTIONS)
