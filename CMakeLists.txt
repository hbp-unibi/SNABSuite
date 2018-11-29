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

# Enable C++14 support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")

# Add configuration option for static linking 
set(STATIC_LINKING TRUE CACHE BOOL "True for static linking")

find_package(PythonLibs 2.7 REQUIRED )

# Link statically
if(STATIC_LINKING)
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	SET(BUILD_SHARED_LIBRARIES OFF)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc  -Wl,--no-export-dynamic")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s -Wl,--no-export-dynamic")
    else()
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
    endif()
endif()


# Add the local "cmake" directory to the search path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# Include the Cypress library
include(cypress_external)

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

set(SNAB_DEBUG FALSE CACHE BOOL "Enable debug output of SNABs")
if(SNAB_DEBUG)
    add_definitions(-DSNAB_DEBUG)
endif()

enable_testing()

include_directories(source/)

#
# Benchmark basic libraries
#

add_library(benchmark_library
	source/util/read_json
	source/util/spiking_utils
	source/util/utilities
    source/common/benchmark
    source/common/neuron_parameters
    source/common/parameter_sweep
	source/common/snab_base
	source/common/snab_registry
    source/SNABs/output_bench
    source/SNABs/refractory_period
    source/SNABs/max_input
    source/SNABs/max_inter_neuron
    source/SNABs/setup_time
    source/SNABs/wta_like
)
target_link_libraries(benchmark_library
    ${CYPRESS_LIBRARY}
)
add_dependencies(benchmark_library cypress_ext)

add_executable(benchmark
    source/exec/snab.cpp
)
add_dependencies(benchmark cypress_ext)

target_link_libraries(benchmark
    benchmark_library
)

add_executable(sweep
    source/exec/snab_sweep.cpp
)
add_dependencies(sweep cypress_ext)

target_link_libraries(sweep
    benchmark_library
)

#
# Copy Matplotlibrc to build directory
# 

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plot/matplotlibrc ${CMAKE_CURRENT_BINARY_DIR}/matplotlibrc COPYONLY)


#
# Unit tests -- only enable if gtest is present
#
if(${GTEST_FOUND})
	add_subdirectory(test)
endif()
