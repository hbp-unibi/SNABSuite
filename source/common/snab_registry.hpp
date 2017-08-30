/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2016  Christoph Jenzen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef SNABSUITE_COMMON_SNAB_REGISTRY_HPP
#define SNABSUITE_COMMON_SNAB_REGISTRY_HPP

#include <memory>
#include <string>
#include <vector>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * A vector containing all SNABs/benchmarks which should be executed. The shared
 * pointer ensures that objects live 'long enough'
 */
std::vector<std::shared_ptr<SNABBase>> snab_registry(std::string backend,
                                                     size_t bench_index);
}

#endif /* SNABSUITE_COMMON_SNAB_REGISTRY_HPP */
