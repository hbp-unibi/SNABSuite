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

#include "common/benchmark_base.hpp"

namespace SNAB {
class SingleMaxFreqToGroup : public BenchmarkBase {
private:
	cypress::PopulationBase m_pop_single, m_pop_group;
	size_t m_num_neurons = 0;
	cypress::Real simulation_length = 150;  // ms
	NeuronParameters m_group_params;

public:
	SingleMaxFreqToGroup(const std::string backend);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
};

class GroupMaxFreqToGroup : public BenchmarkBase {
private:
	cypress::PopulationBase m_pop_max, m_pop_retr;
	size_t m_num_neurons = 0;
	cypress::Real simulation_length = 150;  // ms
	NeuronParameters m_retr_params;

public:
	GroupMaxFreqToGroup(const std::string backend);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
};
}

#endif /* SNABSUITE_SNABS_MAX_INTER_NEURON_HPP */
