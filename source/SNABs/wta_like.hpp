/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2018  Christoph Ostrau
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

#ifndef SNABSUITE_SNABS_WTA_HPP
#define SNABSUITE_SNABS_WTA_HPP

#include <cypress/cypress.hpp>

#include <vector>

#include "common/neuron_parameters.hpp"
#include "common/snab_base.hpp"

namespace SNAB {

/**
 * SimpleWTA: A simple Winner-Takes-All network. Every population represents a
 * winner, inhibition is directly between the populations.
 */
class SimpleWTA : public SNABBase {
private:
	std::vector<cypress::PopulationBase> m_pop;
	std::vector<cypress::Population<cypress::SpikeSourcePoisson>> m_pop_source;
	size_t m_num_neurons_pop = 0, m_num_source_neurons = 0;
	cypress::Real m_firing_rate;
	NeuronParameters m_neuro_params;
	cypress::Real m_simulation_length = 10000;  // ms
	cypress::Real m_bin_size = 15.0;            // ms

	// Network parameters
	cypress::Real m_weight_inp = 0, m_delay = 1.0, m_weight_self = 0.0,
	              m_weight_inh = 0.0;
	cypress::Real m_prob_inp = 0, m_prob_self = 0.0, m_prob_inh = 0.0;

public:
	SimpleWTA(std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SimpleWTA>(m_backend, m_bench_index);
	}

	/**
	 * Calculate the metrics for comparing WTA networks
	 * @param bins summed binned number of spikes of first population
	 * @param bins2 the same for the second population
	 * @return three component vector containing {Length of the longest winning
	 * period in ms, number of state changes, simulation time spent without a
	 * winner}
	 */
	static std::vector<Real> calculate_WTA_metrics(
	    const std::vector<size_t> &bins, const std::vector<size_t> &bins2,
	    const Real bin_size);
};

/**
 * Lateral Inhibition Winner-Takes-All: A centered inhibitory population
 * suppresses all Winner populations.
 */
class LateralInhibWTA : public SNABBase {
private:
	std::vector<cypress::PopulationBase> m_pop;
	std::vector<cypress::Population<cypress::SpikeSourcePoisson>> m_pop_source;
	cypress::PopulationBase m_inhibit_pop;
	size_t m_num_neurons_pop = 0, m_num_source_neurons = 0,
	       m_num_inhibitory_neurons = 0;
	cypress::Real m_firing_rate;
	NeuronParameters m_neuro_params;
	cypress::Real m_simulation_length = 10000;  // ms
	cypress::Real m_bin_size = 15.0;            // ms

	// Network parameters
	cypress::Real m_weight_inp = 0, m_delay = 1.0, m_weight_self = 0.0,
	              m_weight_lat_inh = 0.0, m_weight_lat_exc = 0.0;
	cypress::Real m_prob_inp = 0, m_prob_self = 0.0, m_prob_lat_exc = 0.0;

public:
	LateralInhibWTA(std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &network) override;
	void run_netw(cypress::Network &network) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<LateralInhibWTA>(m_backend, m_bench_index);
	}
};

/**
 * All Winner populations are mirrored by an inhibitory population, which
 * suppresses all other (besides their own) winner population.
 */
class MirrorInhibWTA : public SNABBase {
private:
	std::vector<cypress::PopulationBase> m_pop;
	std::vector<cypress::Population<cypress::SpikeSourcePoisson>> m_pop_source;
	std::vector<cypress::PopulationBase> m_inhibit_pop;
	size_t m_num_neurons_pop = 0, m_num_source_neurons = 0,
	       m_num_inhibitory_neurons = 0;
	cypress::Real m_firing_rate;
	NeuronParameters m_neuro_params;
	cypress::Real m_simulation_length = 10000;  // ms
	cypress::Real m_bin_size = 15.0;            // ms

	// Network parameters
	cypress::Real m_weight_inp = 0, m_delay = 1.0, m_weight_self = 0.0,
	              m_weight_to_inh = 0.0, m_weight_from_inh = 0.0;
	cypress::Real m_prob_inp = 0, m_prob_self = 0.0, m_prob_to_inh = 0.0;

public:
	MirrorInhibWTA(std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &network) override;
	void run_netw(cypress::Network &network) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MirrorInhibWTA>(m_backend, m_bench_index);
	}
};
}  // namespace SNAB

#endif /* SNABSUITE_SNABS_WTA_HPP */
