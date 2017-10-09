/*
 *  SNABSuite - Spiking Neural Architecture Benchmark Suite
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

#include "common/snab_registry.hpp"

#include <memory>
#include <string>
#include <vector>

#include "SNABs/max_input.hpp"
#include "SNABs/max_inter_neuron.hpp"
#include "SNABs/output_bench.hpp"
#include "SNABs/refractory_period.hpp"
#include "common/snab_base.hpp"

namespace SNAB {
std::vector<std::shared_ptr<SNABBase>> snab_registry(std::string backend,
                                                     size_t bench_index)
{
	std::vector<std::shared_ptr<SNABBase>> vec = {
	    std::make_shared<OutputFrequencySingleNeuron>(
	        OutputFrequencySingleNeuron(backend, bench_index)),
	    std::make_shared<OutputFrequencySingleNeuron2>(
	        OutputFrequencySingleNeuron2(backend, bench_index)),
	    std::make_shared<OutputFrequencyMultipleNeurons>(
	        OutputFrequencyMultipleNeurons(backend, bench_index)),
	    std::make_shared<RefractoryPeriod>(
	        RefractoryPeriod(backend, bench_index)),
	    std::make_shared<MaxInputOneToOne>(
	        MaxInputOneToOne(backend, bench_index)),
	    std::make_shared<MaxInputAllToAll>(
	        MaxInputAllToAll(backend, bench_index)),
	    std::make_shared<MaxInputFixedOutConnector>(
	        MaxInputFixedOutConnector(backend, bench_index))/*,
	    std::make_shared<SingleMaxFreqToGroup>(
	        SingleMaxFreqToGroup(backend, bench_index)),
	    std::make_shared<GroupMaxFreqToGroup>(
	        GroupMaxFreqToGroup(backend, bench_index)),
	    std::make_shared<GroupMaxFreqToGroupAllToAll>(
	        GroupMaxFreqToGroupAllToAll(backend, bench_index))*/ };
	return vec;
}
}
