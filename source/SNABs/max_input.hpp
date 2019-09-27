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
#include "common/snab_base.hpp"

namespace SNAB {
/**
 * Check the input bandwidth by injecting spikes per one to one connection.
 * Check if output neurons spike accordingly
 */
class MaxInputOneToOne : public SNABBase {
private:
	cypress::PopulationBase m_pop;
	cypress::Population<cypress::SpikeSourceArray> m_pop_source;
	size_t m_num_neurons = 0, m_num_spikes = 0;
	NeuronParameters m_neuro_params;
	cypress::Real simulation_length = 100;  // ms

public:
	MaxInputOneToOne(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MaxInputOneToOne>(m_backend, m_bench_index);
	}
};

/**
 * Check the input bandwidth by injecting spikes per all to all connection.
 * Check if output neurons spike accordingly
 */
class MaxInputAllToAll : public SNABBase {
private:
	cypress::PopulationBase m_pop;
	cypress::Population<cypress::SpikeSourceArray> m_pop_source;
	size_t m_num_neurons = 0, m_num_inp_neurons = 0, m_num_spikes = 0;
	NeuronParameters m_neuro_params;
	cypress::Real simulation_length = 100;  // ms

public:
	MaxInputAllToAll(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MaxInputAllToAll>(m_backend, m_bench_index);
	}
};

/**
 * Check the input bandwidth by injecting spikes per fixed fan out connection.
 * Check if output neurons spike accordingly
 */
class MaxInputFixedOutConnector : public SNABBase {
protected:
	cypress::PopulationBase m_pop;
	cypress::Population<cypress::SpikeSourceArray> m_pop_source;
	size_t m_num_neurons = 0, m_num_inp_neurons = 0, m_num_spikes = 0;
	NeuronParameters m_neuro_params;
	cypress::Real simulation_length = 100;  // ms
	MaxInputFixedOutConnector(
	    std::string name, std::string backend,
	    std::initializer_list<std::string> indicator_names,
	    std::initializer_list<std::string> indicator_types,
	    std::initializer_list<std::string> indicator_measures,
	    std::initializer_list<std::string> indicator_units,
	    std::initializer_list<std::string> required_parameters,
	    size_t bench_index);

public:
	MaxInputFixedOutConnector(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MaxInputFixedOutConnector>(m_backend,
		                                                   m_bench_index);
	}
};

/**
 * Check the input bandwidth by injecting spikes per fixed fan out connection.
 * Check if output neurons spike accordingly
 */
class MaxInputFixedInConnector : public MaxInputFixedOutConnector {
public:
	MaxInputFixedInConnector(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MaxInputFixedInConnector>(m_backend,
		                                                  m_bench_index);
	}
};
}  // namespace SNAB
#endif /* SNABSUITE_SNABS_MAX_INPUT_HPP */
