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

#pragma once

#ifndef SNABSUITE_SNABS_MAX_INTER_NEURON_HPP
#define SNABSUITE_SNABS_MAX_INTER_NEURON_HPP

#include <cypress/cypress.hpp>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * The aim of this SNAB is to measure the bandwidth between neuron populations.
 * Therefore, taking parameters from the OutputFrequency benchmarks, a single
 * continously spiking neuron is connected to a population of neurons. The
 * number of spikes of the population is measured and taken as the benchmark
 * measure.
 *
 */
class SingleMaxFreqToGroup : public SNABBase {
private:
	cypress::PopulationBase m_pop_single, m_pop_group;
	cypress::Real m_simulation_length = 150;  // ms
	cypress::Real m_start_time = 50;          // ms
	NeuronParameters m_group_params;

public:
	SingleMaxFreqToGroup(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
};

/**
 * Similar to the benchmark before, this SNAB measures the bandwidth between
 * neuron populations. Now, the input population firing at maximal frequency
 * consist of the same number of neurons as the target population. The
 * populations are connected via a OneToOne-Connector.
 */
class GroupMaxFreqToGroup : public SNABBase {
private:
	cypress::PopulationBase m_pop_max, m_pop_retr;
	cypress::Real m_simulation_length = 150;  // ms
	cypress::Real m_start_time = 50;          // ms
	NeuronParameters m_retr_params;

public:
	GroupMaxFreqToGroup(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
};
}

#endif /* SNABSUITE_SNABS_MAX_INTER_NEURON_HPP */
