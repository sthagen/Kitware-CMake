if(DEFINED CMAKE_PARENT_LIST_FILE)
  message(SEND_ERROR "`CMAKE_PARENT_LIST_FILE` is not expected to be set here")
endif()
message(STATUS "ParentVariableScript.cmake: '${CMAKE_PARENT_LIST_FILE}'")
include("${CMAKE_CURRENT_LIST_DIR}/ParentVariableScript/include1.cmake")
if(DEFINED CMAKE_PARENT_LIST_FILE)
  message(SEND_ERROR "`CMAKE_PARENT_LIST_FILE` is not expected to be set here")
endif()
message(STATUS "ParentVariableScript.cmake: '${CMAKE_PARENT_LIST_FILE}'")
