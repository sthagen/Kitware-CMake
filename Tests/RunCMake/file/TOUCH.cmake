set(file "${CMAKE_CURRENT_BINARY_DIR}/file-to-touch")

file(REMOVE "${file}")
file(TOUCH_NOCREATE "${file}")
if(EXISTS "${file}")
  message(FATAL_ERROR "error: TOUCH_NOCREATE created a file!")
endif()

file(TOUCH "${file}")
if(NOT EXISTS "${file}")
  message(FATAL_ERROR "error: TOUCH did not create a file!")
endif()
file(REMOVE "${file}")

# Empty arguments used to be an error, but this is valid since CMake 3.30
file(TOUCH)
file(TOUCH_NOCREATE)
