include(RunCMake)

run_cmake(add_custom_target)
run_cmake(add_custom_command)
run_cmake(add_link_options)
run_cmake(link_directories)
run_cmake(target_link_options)
run_cmake(target_link_directories)
run_cmake(no-arguments)
run_cmake(empty-arguments)
run_cmake(forbidden-arguments)
run_cmake(invalid-feature)
run_cmake(multiple-definitions)
run_cmake(bad-feature1)
run_cmake(bad-feature2)
run_cmake(bad-feature3)
run_cmake(bad-feature4)
run_cmake(bad-feature5)
run_cmake(bad-feature6)
run_cmake(bad-feature7)
run_cmake(feature-not-supported)
run_cmake(library-ignored)
run_cmake(compatible-features)
run_cmake(incompatible-features1)
run_cmake(incompatible-features2)
run_cmake(incompatible-features3)
run_cmake(nested-compatible-features)
run_cmake(nested-incompatible-features)
run_cmake(only-targets)

# testing target propertes LINK_LIBRARY_OVERRIDE and LINK_LIBRARY_OVERRIDE_<LIBRARY>
run_cmake(override-features1)
run_cmake(override-features2)
run_cmake(override-features3)
run_cmake(override-features4)
run_cmake(override-features5)
