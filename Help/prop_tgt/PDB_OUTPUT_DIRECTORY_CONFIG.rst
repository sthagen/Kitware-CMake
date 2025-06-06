PDB_OUTPUT_DIRECTORY_<CONFIG>
-----------------------------

Per-configuration output directory for the MS debug symbol ``.pdb`` file
generated by the linker for an executable or shared library target.

This is a per-configuration version of :prop_tgt:`PDB_OUTPUT_DIRECTORY`,
but multi-configuration generators (:ref:`Visual Studio Generators`,
:generator:`Xcode`) do NOT append a
per-configuration subdirectory to the specified directory.  This
property is initialized by the value of the
:variable:`CMAKE_PDB_OUTPUT_DIRECTORY_<CONFIG>` variable if it is
set when a target is created.

.. versionadded:: 3.12

  Contents of ``PDB_OUTPUT_DIRECTORY_<CONFIG>`` may use
  :manual:`generator expressions <cmake-generator-expressions(7)>`.

.. |COMPILE_PDB_XXX| replace:: :prop_tgt:`COMPILE_PDB_OUTPUT_DIRECTORY_<CONFIG>`
.. include:: include/PDB_NOTE.rst
