set(CMake_TEST_CTestUpdate_BZR "ON" CACHE BOOL "")
set(CMake_TEST_CTestUpdate_CVS "ON" CACHE BOOL "")
set(CMake_TEST_CTestUpdate_GIT "ON" CACHE BOOL "")
set(CMake_TEST_CTestUpdate_HG "ON" CACHE BOOL "")
set(CMake_TEST_CTestUpdate_SVN "ON" CACHE BOOL "")
set(CMake_TEST_FindALSA "ON" CACHE BOOL "")
set(CMake_TEST_FindBLAS "All;static=1;Generic" CACHE STRING "")
set(CMake_TEST_FindBoost "ON" CACHE BOOL "")
set(CMake_TEST_FindBoost_Python "ON" CACHE BOOL "")
set(CMake_TEST_FindBZip2 "ON" CACHE BOOL "")
set(CMake_TEST_FindCups "ON" CACHE BOOL "")
set(CMake_TEST_FindCURL "ON" CACHE BOOL "")
set(CMake_TEST_FindDevIL "ON" CACHE BOOL "")
set(CMake_TEST_FindDoxygen_Dot "ON" CACHE BOOL "")
set(CMake_TEST_FindDoxygen "ON" CACHE BOOL "")
set(CMake_TEST_FindEXPAT "ON" CACHE BOOL "")
set(CMake_TEST_FindFontconfig "ON" CACHE BOOL "")
set(CMake_TEST_FindFreetype "ON" CACHE BOOL "")
set(CMake_TEST_FindGDAL "ON" CACHE BOOL "")
set(CMake_TEST_FindGIF "ON" CACHE BOOL "")
set(CMake_TEST_FindGit "ON" CACHE BOOL "")
set(CMake_TEST_FindGLEW "ON" CACHE BOOL "")
set(CMake_TEST_FindGLUT "ON" CACHE BOOL "")
set(CMake_TEST_FindGnuTLS "ON" CACHE BOOL "")
set(CMake_TEST_FindGSL "ON" CACHE BOOL "")
set(CMake_TEST_FindGTest "ON" CACHE BOOL "")
set(CMake_TEST_FindGTK2 "ON" CACHE BOOL "")
set(CMake_TEST_FindIconv "ON" CACHE BOOL "")
set(CMake_TEST_FindIntl "ON" CACHE BOOL "")
set(CMake_TEST_FindJPEG "ON" CACHE BOOL "")
set(CMake_TEST_FindJsonCpp "ON" CACHE BOOL "")
set(CMake_TEST_FindLAPACK "All;static=1;Generic" CACHE STRING "")
set(CMake_TEST_FindLibArchive "ON" CACHE BOOL "")
set(CMake_TEST_FindLibinput "ON" CACHE BOOL "")
set(CMake_TEST_FindLibLZMA "ON" CACHE BOOL "")
set(CMake_TEST_FindLibUV "ON" CACHE BOOL "")
set(CMake_TEST_FindLibXml2 "ON" CACHE BOOL "")
set(CMake_TEST_FindLibXslt "ON" CACHE BOOL "")
set(CMake_TEST_FindMPI_C "ON" CACHE BOOL "")
set(CMake_TEST_FindMPI_CXX "ON" CACHE BOOL "")
set(CMake_TEST_FindMPI_Fortran "ON" CACHE BOOL "")
set(CMake_TEST_FindMPI "ON" CACHE BOOL "")
set(CMake_TEST_FindODBC "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenACC "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenGL "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenMP_C "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenMP_CXX "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenMP_Fortran "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenMP "ON" CACHE BOOL "")
set(CMake_TEST_FindOpenSSL "ON" CACHE BOOL "")
set(CMake_TEST_FindPatch "ON" CACHE BOOL "")
set(CMake_TEST_FindPNG "ON" CACHE BOOL "")
set(CMake_TEST_FindPostgreSQL "ON" CACHE BOOL "")
set(CMake_TEST_FindProtobuf "ON" CACHE BOOL "")
set(CMake_TEST_FindProtobuf_gRPC "ON" CACHE BOOL "")
set(CMake_TEST_FindPython "ON" CACHE BOOL "")
set(CMake_TEST_FindPython_NumPy "ON" CACHE BOOL "")
set(CMake_TEST_FindPython_PyPy "ON" CACHE BOOL "")
set(CMake_TEST_FindRuby "ON" CACHE BOOL "")
set(CMake_TEST_FindSDL "ON" CACHE BOOL "")
set(CMake_TEST_FindSQLite3 "ON" CACHE BOOL "")
set(CMake_TEST_FindTIFF "ON" CACHE BOOL "")
set(CMake_TEST_FindX11 "ON" CACHE BOOL "")
set(CMake_TEST_FindXalanC "ON" CACHE BOOL "")
set(CMake_TEST_FindXercesC "ON" CACHE BOOL "")
set(CMake_TEST_Fortran_SUBMODULES "ON" CACHE BOOL "")
set(CMake_TEST_IPO_WORKS_C "ON" CACHE BOOL "")
set(CMake_TEST_IPO_WORKS_CXX "ON" CACHE BOOL "")
set(CMake_TEST_IPO_WORKS_Fortran "ON" CACHE BOOL "")
set(CMake_TEST_JQ "/usr/bin/jq" CACHE PATH "")
set(CMake_TEST_Qt5 "ON" CACHE BOOL "")
set(CMake_TEST_UseSWIG "ON" CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_external_test.cmake")
