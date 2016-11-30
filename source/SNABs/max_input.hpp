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

#ifndef SNABSUITE_SNABS_MAX_INPUT_HPP
#define SNABSUITE_SNABS_MAX_INPUT_HPP

#include <cypress/cypress.hpp>

#include "common/neuron_parameters.hpp"
#include "common/benchmark_base.hpp"

namespace SNAB {
/**
 * Check the input bandwidth by injecting spikes per one to one connection.
 * Check if output neurons spike accordingly
 */
class MaxInputOneToOne : public BenchmarkBase {
private:
	cypress::PopulationBase m_pop;
    cypress::Population<cypress::SpikeSourceArray> m_pop_source;
	size_t m_num_neurons = 0, m_num_spikes = 0;
    NeuronParameters m_neuro_params;
    cypress::Real simulation_length = 100;// ms

public:
	MaxInputOneToOne(const std::string backend);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
};
}
#endif /* SNABSUITE_SNABS_MAX_INPUT_HPP */
