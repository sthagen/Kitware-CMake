# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
CheckCSourceRuns
----------------

Check once if given C source compiles and links into an executable and can
subsequently be run.

.. command:: check_c_source_runs

  .. code-block:: cmake

    check_c_source_runs(<code> <resultVar>)

  Check once that the source supplied in ``<code>`` can be built, linked as an
  executable, and then run. The ``<code>`` must contain at least a ``main()``
  function.

  The result is stored in the internal cache variable specified by
  ``<resultVar>``. If the code builds and runs with exit code ``0``, success is
  indicated by boolean ``true``. Failure to build or run is indicated by boolean
  ``false``, such as an empty string or an error message.

  See also :command:`check_source_runs` for a more general command syntax.

  The compile and link commands can be influenced by setting any of the
  following variables prior to calling ``check_c_source_runs()``:

.. include:: /module/include/CMAKE_REQUIRED_FLAGS.rst

.. include:: /module/include/CMAKE_REQUIRED_DEFINITIONS.rst

.. include:: /module/include/CMAKE_REQUIRED_INCLUDES.rst

.. include:: /module/include/CMAKE_REQUIRED_LINK_OPTIONS.rst

.. include:: /module/include/CMAKE_REQUIRED_LIBRARIES.rst

.. include:: /module/include/CMAKE_REQUIRED_LINK_DIRECTORIES.rst

.. include:: /module/include/CMAKE_REQUIRED_QUIET.rst

#]=======================================================================]

include_guard(GLOBAL)
include(Internal/CheckSourceRuns)

macro(CHECK_C_SOURCE_RUNS SOURCE VAR)
  set(_CheckSourceRuns_old_signature 1)
  cmake_check_source_runs(C "${SOURCE}" ${VAR} ${ARGN})
  unset(_CheckSourceRuns_old_signature)
endmacro()
