#  SNABSuite -- Spiking Neural Architecture Benchmark Suite
#  Copyright (C) 2016  Christoph Jenzen
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# CMake version and project metadata
cmake_minimum_required (VERSION 3.2)
project(SNABSuite)

include_directories(PRIVATE ${GTEST_INCLUDE_DIR})

add_executable(SNABSuite_test_util
	util/test_read_json.cpp
	util/test_spiking_utils.cpp
	util/test_utilities.cpp
)
target_link_libraries(SNABSuite_test_util
	benchmark_library
	${GTEST_LIBRARIES}
)
add_test(SNABSuite_test_util SNABSuite_test_util)
add_dependencies(SNABSuite_test_util cypress_ext)

add_executable(SNABSuite_test_common
	common/test_neuron_parameters.cpp
	common/test_parameter_sweep.cpp
)
target_link_libraries(SNABSuite_test_common
	benchmark_library
	${GTEST_LIBRARIES}
)
add_test(SNABSuite_test_common SNABSuite_test_common)
add_dependencies(SNABSuite_test_common cypress_ext)



