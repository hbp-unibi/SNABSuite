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
FetchContent_Declare(cppnam_rep
    GIT_REPOSITORY      "https://github.com/hbp-unibi/cppnam/"
    GIT_TAG             master
)
# FetchContent_MakeAvailable(cppnam_rep)
FetchContent_GetProperties(cppnam_rep)
if(NOT cppnam_rep_POPULATED)
    FetchContent_Populate(cppnam_rep)
endif()
include_directories(
	${cppnam_rep_SOURCE_DIR}/src/
)

add_library(cppnam
	${cppnam_rep_SOURCE_DIR}/src/core/binam
	${cppnam_rep_SOURCE_DIR}/src/core/entropy
	${cppnam_rep_SOURCE_DIR}/src/core/parameters
	${cppnam_rep_SOURCE_DIR}/src/core/spiking_binam
	${cppnam_rep_SOURCE_DIR}/src/core/spiking_netw_basis
	${cppnam_rep_SOURCE_DIR}/src/core/spiking_parameters
	${cppnam_rep_SOURCE_DIR}/src/core/spiking_utils
	${cppnam_rep_SOURCE_DIR}/src/util/binary_matrix
	${cppnam_rep_SOURCE_DIR}/src/util/data
	${cppnam_rep_SOURCE_DIR}/src/util/ncr
	${cppnam_rep_SOURCE_DIR}/src/util/optimisation
	${cppnam_rep_SOURCE_DIR}/src/util/population_count
	${cppnam_rep_SOURCE_DIR}/src/recurrent/rec_binam
	${cppnam_rep_SOURCE_DIR}/src/recurrent/spiking_rec_binam
)
# target_link_libraries(cppnam
# 	${CYPRESS_LIBRARY}
# 	-pthread
# )
add_dependencies(cppnam cypress_ext)

