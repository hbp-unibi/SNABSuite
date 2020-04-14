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

#ifndef SNABSUITE_SNABS_ACTIVATION_CURVE_HPP
#define SNABSUITE_SNABS_ACTIVATION_CURVE_HPP

#include <cypress/cypress.hpp>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * This benchmark is dedicated to compare response/activation functions of
 * neurons in neuromorphic hardware to those from simulation. This being said,
 * the simulation from NEST with a numerical timestep of 0.1ms and complex
 * integrator is used as ground truth.
 */
class WeightDependentActivation : public SNABBase {
protected:
	cypress::PopulationBase m_pop;
	std::vector<cypress::Population<cypress::SpikeSourceArray>> m_pop_source;
	cypress::Real m_offset = 10.0;  // ms; gobal offset
	int num_neurons_per_cycle =
	    20;  // Give the first neuron, that receives input at the same time
	size_t m_num_steps =
	    0;  // Stores the number of steps required to test all weights
	virtual std::vector<std::vector<cypress::Real>> binned_spike_counts();
	WeightDependentActivation(
	    std::string name, std::string backend,
	    std::initializer_list<std::string> indicator_names,
	    std::initializer_list<std::string> indicator_types,
	    std::initializer_list<std::string> indicator_measures,
	    std::initializer_list<std::string> indicator_units,
	    std::initializer_list<std::string> required_parameters,
	    size_t bench_index);

public:
	WeightDependentActivation(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<WeightDependentActivation>(m_backend,
		                                                   m_bench_index);
	}
};

/**
 * This benchmark is dedicated to compare response/activation functions of
 * neurons in neuromorphic hardware to those from simulation. This being said,
 * the simulation from NEST with a numerical timestep of 0.1ms and complex
 * integrator is used as ground truth. Specifically, here we use rates as input
 * to neurons
 */
class RateBasedWeightDependentActivation : public WeightDependentActivation {
protected:
	std::vector<std::vector<cypress::Real>> binned_spike_counts() override;
	cypress::Real fraction_pres_time =
	    0.5;  // Fraction of presentation time used to calculate frequency
public:
	RateBasedWeightDependentActivation(const std::string backend,
	                                   size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<RateBasedWeightDependentActivation>(
		    m_backend, m_bench_index);
	}
};

}  // namespace SNAB
#endif /* SNABSUITE_SNABS_ACTIVATION_CURVE_HPP */
