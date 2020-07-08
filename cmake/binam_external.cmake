#  SNABSuite -- Spiking Neural Architecture Benchmark Suite
#  Copyright (C) 2020 Christoph Ostrau
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


include(FetchContent)
FetchContent_Declare(cppnam
    GIT_REPOSITORY      "https://github.com/hbp-unibi/cppnam/"
    GIT_TAG             master
)
# FetchContent_MakeAvailable(cppnam)
FetchContent_GetProperties(cppnam)
if(NOT cppnam_POPULATED)
    FetchContent_Populate(cppnam)
endif()
include_directories(
	${cppnam_SOURCE_DIR}/src/
)

add_library(libcppnam
	${cppnam_SOURCE_DIR}/src/core/binam
	${cppnam_SOURCE_DIR}/src/core/entropy
	${cppnam_SOURCE_DIR}/src/core/experiment
	${cppnam_SOURCE_DIR}/src/core/parameters
	${cppnam_SOURCE_DIR}/src/core/spiking_binam
	${cppnam_SOURCE_DIR}/src/core/spiking_netw_basis
	${cppnam_SOURCE_DIR}/src/core/spiking_parameters
	${cppnam_SOURCE_DIR}/src/core/spiking_utils
	${cppnam_SOURCE_DIR}/src/util/binary_matrix
	${cppnam_SOURCE_DIR}/src/util/data
	${cppnam_SOURCE_DIR}/src/util/ncr
	${cppnam_SOURCE_DIR}/src/util/optimisation
	${cppnam_SOURCE_DIR}/src/util/population_count
	${cppnam_SOURCE_DIR}/src/recurrent/rec_binam
	${cppnam_SOURCE_DIR}/src/recurrent/spiking_rec_binam
)
target_link_libraries(libcppnam
	${CYPRESS_LIBRARY}
	-pthread
)
add_dependencies(libcppnam cypress_ext)

