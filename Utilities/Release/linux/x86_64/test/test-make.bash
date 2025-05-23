# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

set -e
set -x
mkdir -p /opt/cmake/src/cmake-make
cd /opt/cmake/src/cmake-make
echo >CMakeCache.txt '
CMake_TEST_IPO_WORKS_C:BOOL=ON
CMake_TEST_IPO_WORKS_CXX:BOOL=ON
CMake_TEST_IPO_WORKS_Fortran:BOOL=ON
CMake_TEST_NO_NETWORK:BOOL=ON
CMake_TEST_Qt5:BOOL=ON
'
cmake ../cmake -DCMake_TEST_HOST_CMAKE=1 -G "Unix Makefiles"
make -j $(nproc)
ctest --output-on-failure -j $(nproc)
