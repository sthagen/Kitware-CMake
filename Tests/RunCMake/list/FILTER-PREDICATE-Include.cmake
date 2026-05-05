# Predicate: returns TRUE for items starting with "FILTER_THIS_"
function(starts_with_filter input result)
  if(input MATCHES "^FILTER_THIS_")
    set(${result} TRUE PARENT_SCOPE)
  else()
    set(${result} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Predicate as macro
macro(is_short input result)
  string(LENGTH "${input}" _len)
  if(_len LESS 6)
    set(${result} TRUE)
  else()
    set(${result} FALSE)
  endif()
endmacro()

set(mylist FILTER_THIS_BIT DO_NOT_FILTER_THIS thisisanitem FILTER_THIS_THING)
message("mylist was: ${mylist}")
list(FILTER mylist INCLUDE PREDICATE starts_with_filter)
message("mylist is: ${mylist}")

# INCLUDE with macro predicate
set(mylist ab cdefgh ij klmnop qr)
list(FILTER mylist INCLUDE PREDICATE is_short)
if(NOT mylist STREQUAL "ab;ij;qr")
  message(FATAL_ERROR "FILTER(INCLUDE PREDICATE macro) is \"${mylist}\", expected \"ab;ij;qr\"")
endif()

# INCLUDE on empty list
set(empty_list "")
list(FILTER empty_list INCLUDE PREDICATE starts_with_filter)
if(NOT empty_list STREQUAL "")
  message(FATAL_ERROR "FILTER(INCLUDE PREDICATE empty) is \"${empty_list}\", expected \"\"")
endif()

# INCLUDE where nothing matches (all elements removed)
set(mylist alpha bravo charlie)
list(FILTER mylist INCLUDE PREDICATE starts_with_filter)
if(NOT mylist STREQUAL "")
  message(FATAL_ERROR "FILTER(INCLUDE PREDICATE no-match) is \"${mylist}\", expected \"\"")
endif()

# INCLUDE where everything matches
set(mylist FILTER_THIS_A FILTER_THIS_B FILTER_THIS_C)
list(FILTER mylist INCLUDE PREDICATE starts_with_filter)
if(NOT mylist STREQUAL "FILTER_THIS_A;FILTER_THIS_B;FILTER_THIS_C")
  message(FATAL_ERROR "FILTER(INCLUDE PREDICATE all-match) is \"${mylist}\", expected \"FILTER_THIS_A;FILTER_THIS_B;FILTER_THIS_C\"")
endif()
