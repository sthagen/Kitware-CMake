cmake_minimum_required(VERSION 3.14)
project(TestFindBoostPython CXX)
include(CTest)

find_package(Boost OPTIONAL_COMPONENTS python27 python34 python35 python36 python37 python38 python39 python310 python311 python312 python313 python314)

set(FAILTEST TRUE)
foreach (v IN ITEMS 27 34 35 36 37 38 39 310 311 312 313 314)
  if (Boost_PYTHON${v}_FOUND)
    set(FAILTEST FALSE)
    break()
  endif ()
endforeach ()

if (FAILTEST)
  message(FATAL_ERROR "No Boost Python module found")
endif ()
