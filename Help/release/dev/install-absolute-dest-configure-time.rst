install-absolute-dest-configure-time
-------------------------------------

* CMake gained the ability to diagnose :command:`install` commands that
  specify an absolute ``DESTINATION`` path via the
  :ref:`CMD_INSTALL_ABSOLUTE_DESTINATION <CMD_INSTALL_ABSOLUTE_DESTINATION>`
  diagnostic category. This diagnostic may be controlled with the
  :option:`-Winstall-absolute-destination <cmake -W>` command-line option, the
  ``installAbsoluteDestination`` field in a :manual:`cmake-presets(7)`
  ``warnings`` object, or the :command:`cmake_diagnostic` command.
