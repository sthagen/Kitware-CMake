# Prepare imported targets that the exported project itself imported.
add_library(SubDirLinkAImported IMPORTED INTERFACE)
target_compile_definitions(SubDirLinkAImported INTERFACE DEF_SubDirLinkAImportedForImport)
add_library(SubDirLinkBImported IMPORTED INTERFACE)
target_compile_definitions(SubDirLinkBImported INTERFACE DEF_SubDirLinkBImportedForImport)

# Import targets from the exported build tree.
include(${Import_BINARY_DIR}/../Export/ExportBuildTree.cmake)

# Import targets from the exported install tree.
include(${CMAKE_INSTALL_PREFIX}/lib/exp/exp.cmake)

# Import two exports, where the Depends one depends on an exported target from the Required one:
include(${CMAKE_INSTALL_PREFIX}/lib/cmake/testLibRequired/testLibRequiredTargets.cmake)
include(${CMAKE_INSTALL_PREFIX}/lib/cmake/testLibDepends/testLibDependsTargets.cmake)

# Try referencing an executable imported from the install tree.
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/exp_generated.c
  COMMAND exp_testExe1 ${Import_BINARY_DIR}/exp_generated.c
  DEPENDS exp_testExe1
  )
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/exp_generated3.c
  COMMAND exp_testExe3 ${Import_BINARY_DIR}/exp_generated3.c
  DEPENDS exp_testExe3
  )
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/exp_generated4.c
  COMMAND exp_testExe4 ${Import_BINARY_DIR}/exp_generated4.c
  DEPENDS exp_testExe4
  )

add_executable(imp_testExe1
  imp_testExe1.c
  ${Import_BINARY_DIR}/exp_generated.c
  ${Import_BINARY_DIR}/exp_generated3.c
  ${Import_BINARY_DIR}/exp_generated4.c
  )

function(checkForProperty _TARGET _PROP _EXPECTED)
  get_target_property(EXPORTED_PROPERTY ${_TARGET} "${_PROP}")
  if (NOT EXPORTED_PROPERTY STREQUAL "${_EXPECTED}")
    message(SEND_ERROR "${_TARGET} was expected to export \"${_PROP}\" with value \"${_EXPECTED}\" but got \"${EXPORTED_PROPERTY}\"")
  endif()
endfunction()

checkForProperty(bld_testLib3 "EXPORTED_PROPERTY1"  "EXPORTING_TESTLIB3")
checkForProperty(exp_testLib3 "EXPORTED_PROPERTY1"  "EXPORTING_TESTLIB3")
checkForProperty(bld_testLib4 "EXPORTED_PROPERTY2"  "EXPORTING_TESTLIB4_1")
checkForProperty(exp_testLib4 "EXPORTED_PROPERTY2"  "EXPORTING_TESTLIB4_1")
checkForProperty(bld_testLib4 "EXPORTED_PROPERTY3"  "EXPORTING_TESTLIB4_2")
checkForProperty(exp_testLib4 "EXPORTED_PROPERTY3"  "EXPORTING_TESTLIB4_2")
checkForProperty(bld_testLibDeprecation "DEPRECATION"  "Deprecated version. Please use latest version")
checkForProperty(exp_testLibDeprecation "DEPRECATION"  "Deprecated version. Please use latest version")

# Try linking to a deprecated library
target_link_libraries(imp_testExe1 exp_testLibDeprecation)


# Try linking to a library imported from the install tree.
target_link_libraries(imp_testExe1
  exp_testLib2
  exp_testLib3
  exp_testLib4
  exp_testLib5
  exp_testLib6
  exp_testLib7
  exp_testLibCycleA
  exp_testLibPerConfigDest
  exp_testSharedObjectAIX
  exp_testStaticLibWithPlugin
  )

add_library(imp_testInterfaceInclude1 STATIC imp_testInterfaceInclude1.c)
target_include_directories(imp_testInterfaceInclude1 SYSTEM PRIVATE testInterfaceIncludeSystem)
target_link_libraries(imp_testInterfaceInclude1 PRIVATE exp_testInterfaceIncludeUser)

add_library(imp_testInterfaceInclude1b STATIC imp_testInterfaceInclude1.c)
target_include_directories(imp_testInterfaceInclude1b SYSTEM PRIVATE testInterfaceIncludeSystem)
target_link_libraries(imp_testInterfaceInclude1b PRIVATE exp_testInterfaceIncludeUser2)

add_executable(imp_UseSharedLibWithHelper1 ../../../InterfaceLinkLibrariesDirect/UseSharedLibWithHelper.c)
target_link_libraries(imp_UseSharedLibWithHelper1 PRIVATE exp_testSharedLibWithHelper testSharedLibHelperExclude)

# Try building a plugin to an executable imported from the install tree.
add_library(imp_mod1 MODULE imp_mod1.c)
target_link_libraries(imp_mod1 exp_testExe2)

add_library(imp_ExePlugin1 MODULE ../../../InterfaceLinkLibrariesDirect/ExePlugin.c)
target_link_libraries(imp_ExePlugin1 PRIVATE exp_testExeWithPluginHelper testExePluginHelperExclude)

# Try referencing an executable imported from the build tree.
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/bld_generated.c
  COMMAND bld_testExe1 ${Import_BINARY_DIR}/bld_generated.c
  DEPENDS bld_testExe1
  )
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/bld_generated3.c
  COMMAND bld_testExe3 ${Import_BINARY_DIR}/bld_generated3.c
  DEPENDS bld_testExe3
  )
add_custom_command(
  OUTPUT ${Import_BINARY_DIR}/bld_generated4.c
  COMMAND bld_testExe4 ${Import_BINARY_DIR}/bld_generated4.c
  DEPENDS bld_testExe4
  )

add_executable(imp_testExe1b
  imp_testExe1.c
  ${Import_BINARY_DIR}/bld_generated.c
  ${Import_BINARY_DIR}/bld_generated3.c
  ${Import_BINARY_DIR}/bld_generated4.c
  )

# Try linking to a library imported from the build tree.
target_link_libraries(imp_testExe1b
  bld_testLib2
  bld_testLib3
  bld_testLib4
  bld_testLib5
  bld_testLib6
  bld_testLib7
  bld_testLibCycleA
  bld_testLibPerConfigDest
  bld_testSharedObjectAIX
  bld_testStaticLibWithPlugin
  )

add_library(imp_testInterfaceInclude1c STATIC imp_testInterfaceInclude1.c)
target_include_directories(imp_testInterfaceInclude1c SYSTEM PRIVATE testInterfaceIncludeSystem)
target_link_libraries(imp_testInterfaceInclude1c PRIVATE bld_testInterfaceIncludeUser)

add_library(imp_testInterfaceInclude1d STATIC imp_testInterfaceInclude1.c)
target_include_directories(imp_testInterfaceInclude1d SYSTEM PRIVATE testInterfaceIncludeSystem)
target_link_libraries(imp_testInterfaceInclude1d PRIVATE bld_testInterfaceIncludeUser2)

add_custom_target(check_testLib1_genex ALL
  COMMAND ${CMAKE_COMMAND} -DtestLib1=$<TARGET_FILE:exp_testLib1>
                           -Dprefix=${CMAKE_INSTALL_PREFIX}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/check_testLib1_genex.cmake
  )

if(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG AND
    "${CMAKE_C_CREATE_SHARED_MODULE}" MATCHES "SONAME_FLAG")
  foreach(ns exp bld)
    get_property(configs TARGET ${ns}_testLib5 PROPERTY IMPORTED_CONFIGURATIONS)
    foreach(c ${configs})
      string(TOUPPER "${c}" CONFIG)
      get_property(soname TARGET ${ns}_testLib5 PROPERTY IMPORTED_NO_SONAME_${CONFIG})
      if(soname)
        message(SEND_ERROR "${ns}_testLib5 has IMPORTED_NO_SONAME_${CONFIG} but should:\n  ${soname}")
      else()
        message(STATUS "${ns}_testLib5 does not have IMPORTED_NO_SONAME_${CONFIG} as expected")
      endif()
    endforeach()

    get_property(configs TARGET ${ns}_testLibNoSONAME PROPERTY IMPORTED_CONFIGURATIONS)
    foreach(c ${configs})
      string(TOUPPER "${c}" CONFIG)
      get_property(soname TARGET ${ns}_testLibNoSONAME PROPERTY IMPORTED_NO_SONAME_${CONFIG})
      if(soname)
        message(STATUS "${ns}_testLibNoSONAME has IMPORTED_NO_SONAME_${CONFIG} as expected")
      else()
        message(SEND_ERROR "${ns}_testLibNoSONAME does not have IMPORTED_NO_SONAME_${CONFIG} but should")
      endif()
    endforeach()

    # Parse the binary to check for SONAME if possible.
    if("${CMAKE_EXECUTABLE_FORMAT}" MATCHES "ELF")
      find_program(READELF_EXE readelf)
      if(READELF_EXE)
        add_custom_target(check_${ns}_testLib5_soname ALL COMMAND
          ${CMAKE_COMMAND} -Dreadelf=${READELF_EXE}
          -Dlib=$<TARGET_FILE:${ns}_testLib5>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/check_lib_soname.cmake
          )
        add_custom_target(check_${ns}_testLibNoSONAME_soname ALL COMMAND
          ${CMAKE_COMMAND} -Dreadelf=${READELF_EXE}
          -Dlib=$<TARGET_FILE:${ns}_testLibNoSONAME>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/check_lib_nosoname.cmake
          )
      endif()
    endif()
  endforeach()
endif()

add_executable(cmp0022NEW_test cmp0022NEW_test_vs6_1.cpp)
target_link_libraries(cmp0022NEW_test bld_cmp0022NEW)

add_executable(cmp0022NEW_exp_test cmp0022NEW_test_vs6_2.cpp)
target_link_libraries(cmp0022NEW_exp_test exp_cmp0022NEW)

add_executable(SubDirLink_bld SubDirLink.c)
target_link_libraries(SubDirLink_bld PRIVATE bld_TopDirLib bld_SubDirLinkA)
add_executable(SubDirLink_exp SubDirLink.c)
target_link_libraries(SubDirLink_exp PRIVATE exp_TopDirLib exp_SubDirLinkA)

add_executable(imp_UseSharedLibWithHelper1b ../../../InterfaceLinkLibrariesDirect/UseSharedLibWithHelper.c)
target_link_libraries(imp_UseSharedLibWithHelper1b PRIVATE bld_testSharedLibWithHelper testSharedLibHelperExclude)

# Try building a plugin to an executable imported from the build tree.
add_library(imp_mod1b MODULE imp_mod1.c)
target_link_libraries(imp_mod1b bld_testExe2)

add_library(imp_ExePlugin1b MODULE ../../../InterfaceLinkLibrariesDirect/ExePlugin.c)
target_link_libraries(imp_ExePlugin1b PRIVATE bld_testExeWithPluginHelper testExePluginHelperExclude)

# Export/CMakeLists.txt pretends the RelWithDebInfo (as well as Debug)
# configuration should link to debug libs.
target_compile_definitions(imp_testExe1  PRIVATE "$<$<CONFIG:Debug,RelWithDebInfo>:EXE_DBG>")
target_compile_definitions(imp_testExe1b PRIVATE "$<$<CONFIG:Debug,RelWithDebInfo>:EXE_DBG>")

#-----------------------------------------------------------------------------
include(${CMAKE_INSTALL_PREFIX}/lib/expAbs/expAbs.cmake)

add_executable(imp_testExeAbs1
  imp_testExeAbs1.c
  )
target_link_libraries(imp_testExeAbs1
  expAbs_testLibAbs1
  )

#-----------------------------------------------------------------------------
# Create a custom target to generate a header for the libraries below.
# Drive the header generation through an indirect chain of imported
# target dependencies.

# testLib2tmp1.h
add_custom_command(
  OUTPUT testLib2tmp1.h
  VERBATIM COMMAND
    ${CMAKE_COMMAND} -E echo "extern int testLib2(void);" > testLib2tmp1.h
  )

# hdr_testLib2tmp1 needs testLib2tmp1.h
add_custom_target(hdr_testLib2tmp1 DEPENDS testLib2tmp1.h)

# exp_testExe2 needs hdr_testLib2tmp1
add_dependencies(exp_testExe2 hdr_testLib2tmp1)

# testLib2tmp.h needs exp_testExe2
add_custom_command(
  OUTPUT testLib2tmp.h
  VERBATIM COMMAND exp_testExe2
  COMMAND ${CMAKE_COMMAND} -E copy testLib2tmp1.h testLib2tmp.h
  )

# hdr_testLib2tmp needs testLib2tmp.h
add_custom_target(hdr_testLib2tmp DEPENDS testLib2tmp.h)

add_library(dep_testLib2tmp UNKNOWN IMPORTED)
set_property(TARGET dep_testLib2tmp PROPERTY
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/testLib2tmp.h)

# dep_testLib2tmp needs hdr_testLib2tmp
add_dependencies(dep_testLib2tmp hdr_testLib2tmp)

# testLib2.h needs dep_testLib2tmp
add_custom_command(
  OUTPUT testLib2.h
  VERBATIM COMMAND ${CMAKE_COMMAND} -E copy testLib2tmp.h testLib2.h
  DEPENDS dep_testLib2tmp
  )

# hdr_testLib2 needs testLib2.h
add_custom_target(hdr_testLib2 DEPENDS testLib2.h)

add_library(dep_testLib2 UNKNOWN IMPORTED)

# dep_testLib2 needs hdr_testLib2
add_dependencies(dep_testLib2 hdr_testLib2)

# exp_testLib2 and bld_testLib2 both need dep_testLib2
add_dependencies(bld_testLib2 dep_testLib2)
add_dependencies(exp_testLib2 dep_testLib2)

#-----------------------------------------------------------------------------
# Create a library to be linked by another directory in this project
# to test transitive linking to otherwise invisible imported targets.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_library(imp_lib1 STATIC imp_lib1.c)
target_link_libraries(imp_lib1 exp_testLib2)
add_library(imp_lib1b STATIC imp_lib1.c)
target_link_libraries(imp_lib1b bld_testLib2)

if(NOT CMAKE_GENERATOR STREQUAL "Xcode" OR NOT CMAKE_OSX_ARCHITECTURES MATCHES "[;$]")
  set(bld_objlib_type OBJECT_LIBRARY)

  # Create executables using objects imported from the install tree
  add_executable(imp_testLib8_src imp_testLib8.c $<TARGET_OBJECTS:exp_testLib8>)
  add_executable(imp_testLib8_link imp_testLib8.c)
  target_link_libraries(imp_testLib8_link exp_testLib8)

  if(NOT CMAKE_GENERATOR STREQUAL "Xcode" OR NOT XCODE_VERSION VERSION_LESS 5)
    # Create executables using objects imported from the build tree
    add_executable(imp_testLib8b_src imp_testLib8.c $<TARGET_OBJECTS:bld_testLib8>)
    add_executable(imp_testLib8b_link imp_testLib8.c)
    target_link_libraries(imp_testLib8b_link bld_testLib8)
  endif()
else()
  set(bld_objlib_type INTERFACE_LIBRARY)
endif()

# Create an executable that uses a library imported from the install tree
# that itself was built using an object library.  Verify we get the usage
# requirements.
add_executable(imp_testLib9 imp_testLib9.c)
target_link_libraries(imp_testLib9 exp_testLib9)
# Similarly for importing from the build tree.
add_executable(imp_testLib9b imp_testLib9.c)
target_link_libraries(imp_testLib9b bld_testLib9)

# Check that object libraries were transformed on export as expected.
foreach(vis Pub Priv Iface)
  get_property(type TARGET exp_testLib9Obj${vis} PROPERTY TYPE)
  if(NOT type STREQUAL INTERFACE_LIBRARY)
    message(FATAL_ERROR "exp_testLib9Obj${vis} type is '${type}', not 'INTERFACE_LIBRARY'")
  endif()
  get_property(type TARGET bld_testLib9Obj${vis} PROPERTY TYPE)
  if(NOT type STREQUAL "${bld_objlib_type}")
    message(FATAL_ERROR "bld_testLib9Obj${vis} type is '${type}', not '${bld_objlib_type}'")
  endif()
endforeach()

# Create executables to verify custom transitive properties.
add_executable(imp_testLib10 imp_testLib10.c)
target_link_libraries(imp_testLib10 PRIVATE exp_testLib10)
add_executable(imp_testLib10b imp_testLib10.c)
target_link_libraries(imp_testLib10b PRIVATE bld_testLib10)
add_executable(imp_testLib11 imp_testLib11.c)
target_link_libraries(imp_testLib11 PRIVATE exp_testLib11)
add_executable(imp_testLib11b imp_testLib11.c)
target_link_libraries(imp_testLib11b PRIVATE bld_testLib11)

#-----------------------------------------------------------------------------
# Test that handling imported targets, including transitive dependencies,
# works in CheckFunctionExists (...and hopefully all other try_compile() checks
include(CheckFunctionExists)
unset(HAVE_TESTLIB1_FUNCTION CACHE)
set(CMAKE_REQUIRED_LIBRARIES exp_testLib2)
check_function_exists(testLib1 HAVE_TESTLIB1_FUNCTION)
if (NOT HAVE_TESTLIB1_FUNCTION)
  message(SEND_ERROR "Using imported target testLib2 in check_function_exists() failed !")
endif()

#-----------------------------------------------------------------------------
# Test that dependent imported targets have usable
# INTERFACE_COMPILE_DEFINITIONS and INTERFACE_INCLUDE_DIRECTORIES

add_executable(deps_iface deps_iface.c)
target_link_libraries(deps_iface testLibDepends)

add_executable(deps_shared_iface deps_shared_iface.cpp)
target_link_libraries(deps_shared_iface testSharedLibDepends)
target_compile_definitions(deps_shared_iface
  PRIVATE
    $<$<BOOL:$<TARGET_PROPERTY:POSITION_INDEPENDENT_CODE>>:PIC_PROPERTY_IS_ON>
    $<$<BOOL:$<TARGET_PROPERTY:CUSTOM_PROP>>:CUSTOM_PROPERTY_IS_ON>
    $<$<STREQUAL:$<TARGET_PROPERTY:CUSTOM_STRING>,testcontent>:CUSTOM_STRING_IS_MATCH>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "LCC")
  target_compile_definitions(deps_shared_iface
    PRIVATE
      "DO_GNU_TESTS"
  )
endif()

if (APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "LCC")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-fPIE run_pic_test)
else()
  if (CMAKE_CXX_COMPILER_ID MATCHES "PGI"
      OR CMAKE_CXX_COMPILER_ID MATCHES "PathScale"
      OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(run_pic_test 0)
  else()
    set(run_pic_test 1)
  endif()
endif()

add_executable(exp_renamed_test renamed_test.cpp)
target_link_libraries(exp_renamed_test exp_renamed)

add_executable(bld_renamed_test renamed_test.cpp)
target_link_libraries(bld_renamed_test bld_renamed)

if (run_pic_test)
  target_compile_definitions(deps_shared_iface PRIVATE CHECK_PIC_WORKS)
endif()

if(APPLE)
  add_subdirectory(framework_interface)
endif()

#-----------------------------------------------------------------------------
# Test that targets imported from the build tree have their dependencies
# evaluated correctly. The above already tests the same for the install tree.

add_executable(deps_shared_iface2 deps_shared_iface.cpp)
target_link_libraries(deps_shared_iface2 bld_testSharedLibDepends bld_subdirlib)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "LCC")
  target_compile_definitions(deps_shared_iface2
    PRIVATE
      "DO_GNU_TESTS"
  )
endif()
target_compile_definitions(deps_shared_iface2
  PRIVATE TEST_SUBDIR_LIB
  $<$<BOOL:$<TARGET_PROPERTY:POSITION_INDEPENDENT_CODE>>:PIC_PROPERTY_IS_ON>
  $<$<BOOL:$<TARGET_PROPERTY:CUSTOM_PROP>>:CUSTOM_PROPERTY_IS_ON>
  $<$<STREQUAL:$<TARGET_PROPERTY:CUSTOM_STRING>,testcontent>:CUSTOM_STRING_IS_MATCH>
)

add_subdirectory(excludedFromAll)

add_executable(iface_test_bld iface_test.cpp)
target_link_libraries(iface_test_bld bld_testSharedLibDepends)
set_property(TARGET iface_test_bld PROPERTY NO_SYSTEM_FROM_IMPORTED 1)

set_property(TARGET bld_testSharedLibRequired APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES
        excludedFromAll
)
add_executable(iface_test_exp iface_test.cpp)
target_link_libraries(iface_test_exp testSharedLibDepends)

set_property(TARGET testSharedLibDepends APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES
        excludedFromAll
)

if (((CMAKE_C_COMPILER_ID STREQUAL GNU AND CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.4)
    OR CMAKE_C_COMPILER_ID MATCHES "LCC"
    OR (CMAKE_C_COMPILER_ID STREQUAL Clang AND NOT "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC"))
    AND (CMAKE_GENERATOR STREQUAL "Unix Makefiles" OR CMAKE_GENERATOR STREQUAL "Ninja"))
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wunused-variable run_sys_includes_test)
  if(run_sys_includes_test)
    # The Bullseye wrapper appears to break the -isystem effect.
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if("x${out}" MATCHES "Bullseye")
      set(run_sys_includes_test 0)
    endif()
  endif()
  if (run_sys_includes_test)
    add_executable(test_system_exp test_system.cpp)
    target_link_libraries(test_system_exp exp_systemlib)
    target_compile_options(test_system_exp PRIVATE -Wunused-variable -Werror=unused-variable)

    unset(EXP_ERROR_VARIABLE CACHE)
    try_compile(EXP_ERROR_VARIABLE
      "${CMAKE_CURRENT_BINARY_DIR}/test_system"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
      COMPILE_DEFINITIONS "-Wunused-variable -Werror=unused-variable"
      LINK_LIBRARIES exp_systemlib
      OUTPUT_VARIABLE OUTPUT
      )
    if(NOT EXP_ERROR_VARIABLE)
      message(SEND_ERROR "EXP_ERROR_VARIABLE try_compile failed, but it was expected to succeed ${OUTPUT}.")
    endif()

    if(NOT CMAKE_CROSSCOMPILING)
      unset(EXP_RUN_VAR CACHE)
      unset(EXP_COMPILE_VAR CACHE)
      try_run(EXP_RUN_VAR EXP_COMPILE_VAR
        "${CMAKE_CURRENT_BINARY_DIR}/test_system"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
        COMPILE_DEFINITIONS "-Wunused-variable -Werror=unused-variable"
        LINK_LIBRARIES exp_systemlib
        OUTPUT_VARIABLE OUTPUT
        )
      if(NOT EXP_COMPILE_VAR)
        message(SEND_ERROR "try_run compile failed, but it was expected to succeed ${OUTPUT}.")
      endif()
    endif()

    add_executable(test_system_bld test_system.cpp)
    target_link_libraries(test_system_bld bld_systemlib)
    target_compile_options(test_system_bld PRIVATE -Wunused-variable -Werror=unused-variable)

    unset(BLD_ERROR_VARIABLE CACHE)
    try_compile(BLD_ERROR_VARIABLE
      "${CMAKE_CURRENT_BINARY_DIR}/test_system"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
      COMPILE_DEFINITIONS "-Wunused-variable -Werror=unused-variable"
      LINK_LIBRARIES bld_systemlib
      OUTPUT_VARIABLE OUTPUT
      )
    if(NOT BLD_ERROR_VARIABLE)
      message(SEND_ERROR "BLD_ERROR_VARIABLE try_compile failed, but it was expected to succeed. ${OUTPUT}")
    endif()

    if(NOT CMAKE_CROSSCOMPILING)
      unset(BLD_RUN_VAR CACHE)
      unset(BLD_COMPILE_VAR CACHE)
      try_run(BLD_RUN_VAR BLD_COMPILE_VAR
        "${CMAKE_CURRENT_BINARY_DIR}/test_system"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
        COMPILE_DEFINITIONS "-Wunused-variable -Werror=unused-variable"
        LINK_LIBRARIES bld_systemlib
        OUTPUT_VARIABLE OUTPUT
        )
      if(NOT BLD_COMPILE_VAR)
        message(SEND_ERROR "try_run compile failed, but it was expected to succeed ${OUTPUT}.")
      endif()
    endif()
  endif()

  # Testing try_compile with ALIAS targets.
  # These assume that previous test were successful, or at least the failures will be at the linking stage
  # with symbol not found errors

  # First make sure that if the test run without appropriate alias targets, they should error out
  try_compile(FAILING_LIBRARY_ALIAS_ERROR_VARIABLE
          "${CMAKE_CURRENT_BINARY_DIR}/test_failing_library_alias"
          "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
          LINK_LIBRARIES not_existing_library
          OUTPUT_VARIABLE OUTPUT
          NO_CACHE
  )
  if(FAILING_LIBRARY_ALIAS_ERROR_VARIABLE)
    message(SEND_ERROR "FAILING_LIBRARY_ALIAS_ERROR_VARIABLE try_compile succeeded, but it was expected to fail ${OUTPUT}.")
  endif()

  # FIXME: CMAKE_TRY_COMPILE_TARGET_TYPE=MODULE is needed to properly link and test targets linked to an executable
#  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
#  try_compile(FAILING_EXE_ALIAS_ERROR_VARIABLE
#          "${CMAKE_CURRENT_BINARY_DIR}/test_failing_exe_alias"
#          "${CMAKE_CURRENT_SOURCE_DIR}/imp_mod1.c"
#          LINK_LIBRARIES not_existing_executable
#          OUTPUT_VARIABLE OUTPUT
#          NO_CACHE
#  )
#  unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
#  if(FAILING_EXE_ALIAS_ERROR_VARIABLE)
#    message(SEND_ERROR "FAILING_EXE_ALIAS_ERROR_VARIABLE try_compile succeeded, but it was expected to fail ${OUTPUT}.")
#  endif()

  # Do the actual try_compile tests for ALIAS targets
  add_library(exp_systemlib_alias ALIAS exp_systemlib)
  try_compile(EXP_LIBRARY_ALIAS_ERROR_VARIABLE
          "${CMAKE_CURRENT_BINARY_DIR}/test_library_alias"
          "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
          LINK_LIBRARIES exp_systemlib_alias
          OUTPUT_VARIABLE OUTPUT
          NO_CACHE
  )
  if(NOT EXP_LIBRARY_ALIAS_ERROR_VARIABLE)
    message(SEND_ERROR "EXP_LIBRARY_ALIAS_ERROR_VARIABLE try_compile failed with library aliased target, but it was expected to succeed ${OUTPUT}.")
  endif()

  # FIXME: CMAKE_TRY_COMPILE_TARGET_TYPE=MODULE is needed to properly link and test targets linked to an executable
#  set(CMAKE_TRY_COMPILE_TARGET_TYPE MODULE)
#  add_executable(exp_exe_alias ALIAS exp_testExe2)
#  try_compile(EXP_EXE_ALIAS_ERROR_VARIABLE
#          "${CMAKE_CURRENT_BINARY_DIR}/test_exe_alias"
#          "${CMAKE_CURRENT_SOURCE_DIR}/imp_mod1.c"
#          LINK_LIBRARIES exp_exe_alias
#          OUTPUT_VARIABLE OUTPUT
#          NO_CACHE
#  )
#  unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
#  if(NOT EXP_EXE_ALIAS_ERROR_VARIABLE)
#    message(SEND_ERROR "EXP_EXE_ALIAS_ERROR_VARIABLE try_compile failed with executable aliased target, but it was expected to succeed ${OUTPUT}.")
#  endif()

  add_library(bld_systemlib_alias ALIAS bld_systemlib)
  try_compile(BLD_LIBRARY_ALIAS_ERROR_VARIABLE
          "${CMAKE_CURRENT_BINARY_DIR}/test_library_alias"
          "${CMAKE_CURRENT_SOURCE_DIR}/test_system.cpp"
          LINK_LIBRARIES bld_systemlib_alias
          OUTPUT_VARIABLE OUTPUT
          NO_CACHE
  )
  if(NOT BLD_LIBRARY_ALIAS_ERROR_VARIABLE)
    message(SEND_ERROR "BLD_LIBRARY_ALIAS_ERROR_VARIABLE try_compile failed with library aliased target, but it was expected to succeed. ${OUTPUT}")
  endif()

  # FIXME: CMAKE_TRY_COMPILE_TARGET_TYPE=MODULE is needed to properly link and test targets linked to an executable
#  set(CMAKE_TRY_COMPILE_TARGET_TYPE MODULE)
#  add_executable(bld_exe_alias ALIAS bld_testExe2)
#  try_compile(BLD_EXE_ALIAS_ERROR_VARIABLE
#          "${CMAKE_CURRENT_BINARY_DIR}/test_exe_alias"
#          "${CMAKE_CURRENT_SOURCE_DIR}/imp_mod1.c"
#          LINK_LIBRARIES bld_exe_alias
#          OUTPUT_VARIABLE OUTPUT
#          NO_CACHE
#  )
#  unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
#  if(NOT BLD_EXE_ALIAS_ERROR_VARIABLE)
#    message(SEND_ERROR "BLD_EXE_ALIAS_ERROR_VARIABLE try_compile failed with executable aliased target, but it was expected to succeed. ${OUTPUT}")
#  endif()
endif()

#---------------------------------------------------------------------------------
# check that imported libraries have the expected INTERFACE_LINK_OPTIONS property
checkForProperty(bld_testLinkOptions "INTERFACE_LINK_OPTIONS" "INTERFACE_FLAG")
checkForProperty(Req::testLinkOptions "INTERFACE_LINK_OPTIONS" "INTERFACE_FLAG")

#---------------------------------------------------------------------------------
# check that imported libraries have the expected INTERFACE_LINK_DIRECTORIES property
checkForProperty(bld_testLinkDirectories "INTERFACE_LINK_DIRECTORIES" "/interface/build")
checkForProperty(Req::testLinkDirectories "INTERFACE_LINK_DIRECTORIES" "${CMAKE_INSTALL_PREFIX}/interface/install")

#---------------------------------------------------------------------------------
# check that imported libraries have the expected INTERFACE_LINK_DEPENDS property
if(CMAKE_GENERATOR MATCHES "Make|Ninja")
  checkForProperty(bld_testLinkDepends "INTERFACE_LINK_DEPENDS" "BUILD_LINK_DEPENDS")
  checkForProperty(Req::testLinkDepends "INTERFACE_LINK_DEPENDS" "${CMAKE_INSTALL_PREFIX}/INSTALL_LINK_DEPENDS")
endif()

#------------------------------------------------------------------------------
# test import of CUDA language level
if(CMake_TEST_CUDA)
  checkForProperty(bld_cudaInterfaceLib "INTERFACE_COMPILE_FEATURES" "cuda_std_11")
  checkForProperty(Req::cudaInterfaceLib "INTERFACE_COMPILE_FEATURES" "cuda_std_14")
endif()
