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

#ifndef SNABSUITE_COMMON_BENCHMARK_REGISTRY_HPP
#define SNABSUITE_COMMON_BENCHMARK_REGISTRY_HPP

#include <memory>
#include <string>
#include <vector>

#include "SNABs/max_input.hpp"
#include "SNABs/max_inter_neuron.hpp"
#include "SNABs/output_bench.hpp"
#include "SNABs/refractory_period.hpp"
#include "common/benchmark_base.hpp"

namespace SNAB {
/**
 * A vector containing all benchmarks which should be executed. The shared
 * pointer ensures that objects live 'long enough'
 */
std::vector<std::shared_ptr<BenchmarkBase>> benchmark_registry(
    std::string backend)
{
	std::vector<std::shared_ptr<BenchmarkBase>> vec = {
	    std::make_shared<OutputFrequencySingleNeuron>(
	        OutputFrequencySingleNeuron(backend)),
	    std::make_shared<OutputFrequencyMultipleNeurons>(
	        OutputFrequencyMultipleNeurons(backend)),
	    std::make_shared<RefractoryPeriod>(RefractoryPeriod(backend)),
	    std::make_shared<MaxInputOneToOne>(MaxInputOneToOne(backend)),
	    std::make_shared<MaxInputAllToAll>(MaxInputAllToAll(backend)),
	    std::make_shared<SingleMaxFreqToGroup>(SingleMaxFreqToGroup(backend))};
	return vec;
}
}

#endif /* SNABSUITE_COMMON_BENCHMARK_REGISTRY_HPP */
