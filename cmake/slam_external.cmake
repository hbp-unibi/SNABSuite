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
FetchContent_Declare(slam_rep
    GIT_REPOSITORY      "https://github.com/hbp-unibi/Spiking-Neural-Network-based-SLAM"
    GIT_TAG             master
)
FetchContent_GetProperties(slam_rep)
if(NOT slam_rep_POPULATED)
    FetchContent_Populate(slam_rep)
endif()
include_directories(
	${slam_rep_SOURCE_DIR}/source/
)
add_library(slam
    ${slam_rep_SOURCE_DIR}/source/simulation
    ${slam_rep_SOURCE_DIR}/source/spikingnetwork
)

# target_link_libraries(slam
# 	${CYPRESS_LIBRARY}
# 	-pthread
# )
add_dependencies(slam cypress_ext)
