#  Neuromorphic Benchmarks
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
project(cppnam)

# Link statically
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

# Add the local "cmake" directory to the search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Automatically download and compile Google-Test and the JSON library
include(gtest_external)

# Include the Cypress library
find_package(Cypress REQUIRED)
include_directories(${CYPRESS_INCLUDE_DIR})

# Enable C++14 support
set(CMAKE_CXX_STANDARD 14)

# Enable all warnings on MSVC and GCC/Clang/Intel
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") OR
		("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR
		("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Set a default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

enable_testing()

include_directories(source/)

#
# Benchmark basic libraries
#

add_library(benchmark_utils
	source/util/read_json
	source/util/spiking_utils
	source/util/utilities
)
target_link_libraries(benchmark_utils
	cypress
	-pthread
)

add_library(benchmark_common
    source/common/benchmark
	source/common/benchmark_base
	source/common/benchmark_registry
	source/common/neuron_parameters
)
target_link_libraries(benchmark_common
	cypress
	benchmark_utils
	-pthread
)

add_library(benchmarks 
    source/SNABs/output_bench
)
target_link_libraries(benchmarks
	cypress
	benchmark_common
	benchmark_utils
	-pthread
)

add_executable(benchmark
    source/exec/snab.cpp
)

target_link_libraries(benchmark
    benchmarks
)





#
# Unit tests -- only enable if gtest is present
#
if(${GTEST_FOUND})
	#add_subdirectory(test)
endif()
