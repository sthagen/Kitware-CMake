# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindBISON
---------

Find ``bison`` executable and provide a macro to generate custom build rules.

The module defines the following variables:

``BISON_FOUND``
  True if the program was found.

``BISON_EXECUTABLE``
  The path to the ``bison`` program.

``BISON_VERSION``
  The version of ``bison``.

The minimum required version of ``bison`` can be specified using the
standard CMake syntax, e.g. :command:`find_package(BISON 2.1.3)`.

If ``bison`` is found, the module defines the macro:

.. command:: bison_target

  .. code-block:: cmake

    bison_target(<Name> <YaccInput> <CodeOutput>
                 [OPTIONS <options>...]
                 [COMPILE_FLAGS <string>]
                 [DEFINES_FILE <file>]
                 [VERBOSE [<file>]]
                 [REPORT_FILE <file>]
                 )

which will create a custom rule to generate a parser.  ``<YaccInput>`` is
the path to a yacc file.  ``<CodeOutput>`` is the name of the source file
generated by bison.  A header file can also be generated, and contains
the token list.

.. versionchanged:: 3.14
  When :policy:`CMP0088` is set to ``NEW``, ``bison`` runs in the
  :variable:`CMAKE_CURRENT_BINARY_DIR` directory.

The options are:

``OPTIONS <options>...``
  .. versionadded:: 4.0

  A :ref:`semicolon-separated list <CMake Language Lists>` of options added to
  the ``bison`` command line.

``COMPILE_FLAGS <string>``
  .. deprecated:: 4.0

  Space-separated bison options added to the ``bison`` command line.
  A :ref:`;-list <CMake Language Lists>` will not work.
  This option is deprecated in favor of ``OPTIONS <options>...``.

``DEFINES_FILE <file>``
  .. versionadded:: 3.4

  Specify a non-default header ``<file>`` to be generated by ``bison``.

``VERBOSE [<file>]``
  Tell ``bison`` to write a report file of the grammar and parser.

  .. deprecated:: 3.7
    If ``<file>`` is given, it specifies path the report file is copied to.
    ``[<file>]`` is left for backward compatibility of this module.
    Use ``VERBOSE REPORT_FILE <file>``.

``REPORT_FILE <file>``
  .. versionadded:: 3.7

  Specify a non-default report ``<file>``, if generated.

The macro defines the following variables:

``BISON_<Name>_DEFINED``
  True if the macro ran successfully.

``BISON_<Name>_INPUT``
  The input source file, an alias for ``<YaccInput>``.

``BISON_<Name>_OUTPUT_SOURCE``
  The source file generated by ``bison``.

``BISON_<Name>_OUTPUT_HEADER``
  The header file generated by ``bison``.

``BISON_<Name>_OUTPUTS``
  All files generated by ``bison`` including the source, the header and the
  report.

``BISON_<Name>_OPTIONS``
  .. versionadded:: 4.0

  Options used in the ``bison`` command line.

``BISON_<Name>_COMPILE_FLAGS``
  .. deprecated:: 4.0

  Options used in the ``bison`` command line. This variable is deprecated in
  favor of ``BISON_<Name>_OPTIONS`` variable.

Examples
^^^^^^^^

.. code-block:: cmake

  find_package(BISON)
  bison_target(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
               DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)
  add_executable(Foo main.cpp ${BISON_MyParser_OUTPUTS})

Adding additional command-line options to the ``bison`` executable can be passed
as a list. For example, adding the ``-Wall`` option to report all warnings, and
``--no-lines`` (``-l``) to not generate ``#line`` directives.

.. code-block:: cmake

  find_package(BISON)

  if(BISON_FOUND)
    bison_target(MyParser parser.y parser.cpp OPTIONS -Wall --no-lines)
  endif()

Generator expressions can be used in ``OPTIONS <options...``. For example, to
add the ``--debug`` (``-t``) option only for the ``Debug`` build type:

.. code-block:: cmake

  find_package(BISON)

  if(BISON_FOUND)
    bison_target(MyParser parser.y parser.cpp OPTIONS $<$<CONFIG:Debug>:-t>)
  endif()
#]=======================================================================]

find_program(BISON_EXECUTABLE NAMES bison win-bison win_bison DOC "path to the bison executable")
mark_as_advanced(BISON_EXECUTABLE)

if(BISON_EXECUTABLE)
  # the bison commands should be executed with the C locale, otherwise
  # the message (which are parsed) may be translated
  set(_Bison_SAVED_LC_ALL "$ENV{LC_ALL}")
  set(ENV{LC_ALL} C)

  execute_process(COMMAND ${BISON_EXECUTABLE} --version
    OUTPUT_VARIABLE BISON_version_output
    ERROR_VARIABLE BISON_version_error
    RESULT_VARIABLE BISON_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(ENV{LC_ALL} ${_Bison_SAVED_LC_ALL})

  if(NOT ${BISON_version_result} EQUAL 0)
    message(SEND_ERROR "Command \"${BISON_EXECUTABLE} --version\" failed with output:\n${BISON_version_error}")
  else()
    # Bison++
    if("${BISON_version_output}" MATCHES "^bison\\+\\+ Version ([^,]+)")
      set(BISON_VERSION "${CMAKE_MATCH_1}")
    # GNU Bison
    elseif("${BISON_version_output}" MATCHES "^bison \\(GNU Bison\\) ([^\n]+)\n")
      set(BISON_VERSION "${CMAKE_MATCH_1}")
    elseif("${BISON_version_output}" MATCHES "^GNU Bison (version )?([^\n]+)")
      set(BISON_VERSION "${CMAKE_MATCH_2}")
    endif()
  endif()

  # internal macro
  # sets BISON_TARGET_cmdopt
  macro(BISON_TARGET_option_extraopts Options)
    set(BISON_TARGET_cmdopt "")
    set(BISON_TARGET_extraopts "${Options}")
    separate_arguments(BISON_TARGET_extraopts)
    list(APPEND BISON_TARGET_cmdopt ${BISON_TARGET_extraopts})
  endmacro()

  # internal macro
  # sets BISON_TARGET_output_header and BISON_TARGET_cmdopt
  macro(BISON_TARGET_option_defines BisonOutput Header)
    if("${Header}" STREQUAL "")
      # default header path generated by bison (see option -d)
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\2" _fileext "${BisonOutput}")
      string(REPLACE "c" "h" _fileext ${_fileext})
      string(REGEX REPLACE "^(.*)(\\.[^.]*)$" "\\1${_fileext}"
          BISON_TARGET_output_header "${BisonOutput}")
      list(APPEND BISON_TARGET_cmdopt "-d")
    else()
      set(BISON_TARGET_output_header "${Header}")
      list(APPEND BISON_TARGET_cmdopt "--defines=${BISON_TARGET_output_header}")
    endif()
  endmacro()

  # internal macro
  # sets BISON_TARGET_verbose_file and BISON_TARGET_cmdopt
  macro(BISON_TARGET_option_report_file BisonOutput ReportFile)
    if("${ReportFile}" STREQUAL "")
      get_filename_component(BISON_TARGET_output_path "${BisonOutput}" PATH)
      get_filename_component(BISON_TARGET_output_name "${BisonOutput}" NAME_WE)
      set(BISON_TARGET_verbose_file
        "${BISON_TARGET_output_path}/${BISON_TARGET_output_name}.output")
    else()
      set(BISON_TARGET_verbose_file "${ReportFile}")
      list(APPEND BISON_TARGET_cmdopt "--report-file=${BISON_TARGET_verbose_file}")
    endif()
    if(NOT IS_ABSOLUTE "${BISON_TARGET_verbose_file}")
      cmake_policy(GET CMP0088 _BISON_CMP0088
        PARENT_SCOPE # undocumented, do not use outside of CMake
        )
      if("x${_BISON_CMP0088}x" STREQUAL "xNEWx")
        set(BISON_TARGET_verbose_file "${CMAKE_CURRENT_BINARY_DIR}/${BISON_TARGET_verbose_file}")
      else()
        set(BISON_TARGET_verbose_file "${CMAKE_CURRENT_SOURCE_DIR}/${BISON_TARGET_verbose_file}")
      endif()
      unset(_BISON_CMP0088)
    endif()
  endmacro()

  # internal macro
  # adds a custom command and sets
  #   BISON_TARGET_cmdopt, BISON_TARGET_extraoutputs
  macro(BISON_TARGET_option_verbose Name BisonOutput filename)
    cmake_policy(GET CMP0088 _BISON_CMP0088
        PARENT_SCOPE # undocumented, do not use outside of CMake
        )
    set(_BISON_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if("x${_BISON_CMP0088}x" STREQUAL "xNEWx")
      set(_BISON_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    unset(_BISON_CMP0088)

    list(APPEND BISON_TARGET_cmdopt "--verbose")
    list(APPEND BISON_TARGET_outputs
      "${BISON_TARGET_verbose_file}")
    if (NOT "${filename}" STREQUAL "")
      if(IS_ABSOLUTE "${filename}")
        set(BISON_TARGET_verbose_extra_file "${filename}")
      else()
        set(BISON_TARGET_verbose_extra_file "${_BISON_WORKING_DIRECTORY}/${filename}")
      endif()

      add_custom_command(OUTPUT ${BISON_TARGET_verbose_extra_file}
        COMMAND ${CMAKE_COMMAND} -E copy
        "${BISON_TARGET_verbose_file}"
        "${filename}"
        VERBATIM
        DEPENDS
        "${BISON_TARGET_verbose_file}"
        COMMENT "[BISON][${Name}] Copying bison verbose table to ${filename}"
        WORKING_DIRECTORY ${_BISON_WORKING_DIRECTORY})
      list(APPEND BISON_TARGET_extraoutputs
        "${BISON_TARGET_verbose_extra_file}")
      unset(BISON_TARGET_verbose_extra_file)
      unset(_BISON_WORKING_DIRECTORY)
    endif()
  endmacro()

  #============================================================
  # BISON_TARGET (public macro)
  #============================================================
  #
  macro(BISON_TARGET Name BisonInput BisonOutput)
    set(BISON_TARGET_outputs "${BisonOutput}")
    set(BISON_TARGET_extraoutputs "")

    # Parsing parameters
    set(BISON_TARGET_PARAM_OPTIONS
      )
    set(BISON_TARGET_PARAM_ONE_VALUE_KEYWORDS
      COMPILE_FLAGS
      DEFINES_FILE
      REPORT_FILE
      )
    set(BISON_TARGET_PARAM_MULTI_VALUE_KEYWORDS
      OPTIONS
      VERBOSE
      )
    cmake_parse_arguments(
        BISON_TARGET_ARG
        "${BISON_TARGET_PARAM_OPTIONS}"
        "${BISON_TARGET_PARAM_ONE_VALUE_KEYWORDS}"
        "${BISON_TARGET_PARAM_MULTI_VALUE_KEYWORDS}"
        ${ARGN}
    )

    if(NOT "${BISON_TARGET_ARG_UNPARSED_ARGUMENTS}" STREQUAL "")
      message(SEND_ERROR "Usage")
    elseif("${BISON_TARGET_ARG_VERBOSE}" MATCHES ";")
      # [VERBOSE [<file>] hack: <file> is non-multi value by usage
      message(SEND_ERROR "Usage")
    else()

      BISON_TARGET_option_extraopts("${BISON_TARGET_ARG_COMPILE_FLAGS}")

      if(BISON_TARGET_ARG_OPTIONS)
        list(APPEND BISON_TARGET_cmdopt ${BISON_TARGET_ARG_OPTIONS})
      endif()

      BISON_TARGET_option_defines("${BisonOutput}" "${BISON_TARGET_ARG_DEFINES_FILE}")
      BISON_TARGET_option_report_file("${BisonOutput}" "${BISON_TARGET_ARG_REPORT_FILE}")
      if(NOT "${BISON_TARGET_ARG_VERBOSE}" STREQUAL "")
        BISON_TARGET_option_verbose(${Name} ${BisonOutput} "${BISON_TARGET_ARG_VERBOSE}")
      else()
        # [VERBOSE [<file>]] is used with no argument or is not used
        set(BISON_TARGET_args "${ARGN}")
        list(FIND BISON_TARGET_args "VERBOSE" BISON_TARGET_args_indexof_verbose)
        if(${BISON_TARGET_args_indexof_verbose} GREATER -1)
          # VERBOSE is used without <file>
          BISON_TARGET_option_verbose(${Name} ${BisonOutput} "")
        endif()
      endif()

      list(APPEND BISON_TARGET_outputs "${BISON_TARGET_output_header}")

      cmake_policy(GET CMP0088 _BISON_CMP0088
        PARENT_SCOPE # undocumented, do not use outside of CMake
        )
      set(_BISON_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      set(_BisonInput "${BisonInput}")
      if("x${_BISON_CMP0088}x" STREQUAL "xNEWx")
        set(_BISON_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        if(NOT IS_ABSOLUTE "${_BisonInput}")
          set(_BisonInput "${CMAKE_CURRENT_SOURCE_DIR}/${_BisonInput}")
        endif()
      endif()
      unset(_BISON_CMP0088)

      # Bison cannot create output directories. Create any missing determined
      # directories where the files will be generated if they don't exist yet.
      set(_BisonMakeDirectoryCommand "")
      foreach(output IN LISTS BISON_TARGET_outputs)
        cmake_path(GET output PARENT_PATH dir)
        if(dir)
          list(APPEND _BisonMakeDirectoryCommand ${dir})
        endif()
        unset(dir)
      endforeach()
      if(_BisonMakeDirectoryCommand)
        list(REMOVE_DUPLICATES _BisonMakeDirectoryCommand)
        list(
          PREPEND
          _BisonMakeDirectoryCommand
          COMMAND ${CMAKE_COMMAND} -E make_directory
        )
      endif()

      add_custom_command(OUTPUT ${BISON_TARGET_outputs}
        ${_BisonMakeDirectoryCommand}
        COMMAND ${BISON_EXECUTABLE} ${BISON_TARGET_cmdopt} -o ${BisonOutput} ${_BisonInput}
        VERBATIM
        DEPENDS ${_BisonInput}
        COMMENT "[BISON][${Name}] Building parser with bison ${BISON_VERSION}"
        WORKING_DIRECTORY ${_BISON_WORKING_DIRECTORY}
        COMMAND_EXPAND_LISTS)

      unset(_BISON_WORKING_DIRECTORY)

      # define target variables
      set(BISON_${Name}_DEFINED TRUE)
      set(BISON_${Name}_INPUT ${_BisonInput})
      set(BISON_${Name}_OUTPUTS ${BISON_TARGET_outputs} ${BISON_TARGET_extraoutputs})
      set(BISON_${Name}_OPTIONS ${BISON_TARGET_cmdopt})
      set(BISON_${Name}_COMPILE_FLAGS ${BISON_TARGET_cmdopt})
      set(BISON_${Name}_OUTPUT_SOURCE "${BisonOutput}")
      set(BISON_${Name}_OUTPUT_HEADER "${BISON_TARGET_output_header}")

      unset(_BisonInput)
      unset(_BisonMakeDirectoryCommand)
    endif()
  endmacro()
  #
  #============================================================

endif()

include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BISON REQUIRED_VARS  BISON_EXECUTABLE
                                        VERSION_VAR BISON_VERSION)
