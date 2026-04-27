string(
  REGEX MATCHALL
  "swiftc(\\.exe)?\"? [^\n]* -emit-module-path [^\n]*L\\.swiftmodule"
  swift_module_commands "${actual_stdout}")
list(LENGTH swift_module_commands swift_module_command_count)
if(swift_module_command_count LESS 2)
  string(APPEND RunCMake_TEST_FAILED
    "Expected separate compile and emit-module commands for L, found ${swift_module_command_count} command(s) with '-emit-module-path ... L.swiftmodule'.\n")
endif()
