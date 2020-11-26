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
FetchContent_Declare(eigen
    GIT_REPOSITORY      "https://gitlab.com/libeigen/eigen/"
    GIT_TAG             3.3.8
)
FetchContent_GetProperties(eigen)
if(NOT eigen_POPULATED)
    FetchContent_Populate(eigen)
endif()
include_directories(
	${eigen_SOURCE_DIR}/
)

