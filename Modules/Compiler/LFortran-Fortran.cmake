string(APPEND CMAKE_Fortran_FLAGS_DEBUG_INIT " -g")
string(APPEND CMAKE_Fortran_FLAGS_MINSIZEREL_INIT " ")
string(APPEND CMAKE_Fortran_FLAGS_RELEASE_INIT " -O3")
string(APPEND CMAKE_Fortran_FLAGS_RELWITHDEBINFO_INIT " -O2 -g")
set(CMAKE_Fortran_MODDIR_FLAG "-J")
set(CMAKE_Fortran_VERBOSE_FLAG "-v -Wl,-v")
set(CMAKE_Fortran_FORMAT_FIXED_FLAG "--fixed-form")
set(CMAKE_Fortran_LINKER_WRAPPER_FLAG "-Wl,")
set(CMAKE_Fortran_COMPILE_OPTIONS_PREPROCESS_ON "--cpp")
set(CMAKE_Fortran_COMPILE_OPTIONS_PREPROCESS_OFF "--no-cpp")
set(CMAKE_Fortran_PREPROCESS_SOURCE "<CMAKE_Fortran_COMPILER> --cpp <DEFINES> <INCLUDES> <FLAGS> -E <SOURCE> > <PREPROCESSED_SOURCE>")
set(CMAKE_Fortran_COMPILE_OBJECT "<CMAKE_Fortran_COMPILER> --cpp-infer <DEFINES> <INCLUDES> <FLAGS> --generate-object-code -c <SOURCE> -o <OBJECT>")
set(CMAKE_SHARED_LIBRARY_CREATE_Fortran_FLAGS "--shared")
set(CMAKE_SHARED_LIBRARY_LINK_Fortran_FLAGS "-Wl,-export-dynamic")
