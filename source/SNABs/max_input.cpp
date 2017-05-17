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

#include <memory>
#include <random>
#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank
#include <cypress/cypress.hpp>               // Neural network frontend

#include "common/neuron_parameters.hpp"
#include "max_input.hpp"
#include "util/spiking_utils.hpp"

namespace SNAB {

MaxInputOneToOne::MaxInputOneToOne(const std::string backend)
    : SNABBase(
          __func__, backend, {"Average number of sikes", "Standard deviation",
                              "Maximum #spikes", "Minimum #spikes"},
          {"quality", "quality", "quality", "quality"}, {"", "", "", ""},
          {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes"}),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &MaxInputOneToOne::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	m_num_neurons = m_config_file["#neurons"];
	m_num_spikes = m_config_file["#spikes"];

	// Get neuron neuron_parameters
	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(
		    10.0 + cypress::Real(i) * simulation_length / m_num_spikes);
	}
	m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
	    m_num_neurons, SpikeSourceArrayParameters(spike_times));
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::one_to_one(cypress::Real(m_config_file["weight"])));
	return netw;
}
void MaxInputOneToOne::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<cypress::Real> MaxInputOneToOne::evaluate()
{
	// Gather the average #spikes of every neuron
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}

	// Calculate statistics
	cypress::Real max = *std::max_element(spikes.begin(), spikes.end());
	cypress::Real min = *std::min_element(spikes.begin(), spikes.end());
	cypress::Real avg = std::accumulate(spikes.begin(), spikes.end(), 0.0) /
	                    cypress::Real(spikes.size());
	cypress::Real std_dev = 0.0;
	std::for_each(spikes.begin(), spikes.end(), [&](const cypress::Real val) {
		std_dev += (val - avg) * (val - avg);
	});
	std_dev = std::sqrt(std_dev / cypress::Real(spikes.size() - 1));

	std::vector<cypress::Real> results = {avg, std_dev, max, min};
	return results;
}

MaxInputAllToAll::MaxInputAllToAll(const std::string backend)
    : SNABBase(__func__, backend,
               {"Average number of sikes", "Standard deviation",
                "Maximum #spikes", "Minimum #spikes"},
               {"quality", "quality", "quality", "quality"}, {"", "", "", ""},
               {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes",
                "#input_neurons"}),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &MaxInputAllToAll::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	m_num_neurons = m_config_file["#neurons"];
	m_num_spikes = m_config_file["#spikes"];
	m_num_inp_neurons = m_config_file["#input_neurons"];

	// Get neuron neuron_parameters
	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(
		    10.0 + cypress::Real(i) * simulation_length / m_num_spikes);
	}
	m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
	    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times));
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}
void MaxInputAllToAll::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<cypress::Real> MaxInputAllToAll::evaluate()
{
	// Gather the average #spikes of every neuron
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}

	// Calculate statistics
	cypress::Real max = *std::max_element(spikes.begin(), spikes.end());
	cypress::Real min = *std::min_element(spikes.begin(), spikes.end());
	cypress::Real avg = std::accumulate(spikes.begin(), spikes.end(), 0.0) /
	                    cypress::Real(spikes.size());
	cypress::Real std_dev = 0.0;
	std::for_each(spikes.begin(), spikes.end(), [&](const cypress::Real val) {
		std_dev += (val - avg) * (val - avg);
	});
	std_dev = std::sqrt(std_dev / cypress::Real(spikes.size() - 1));

	std::vector<cypress::Real> results = {avg, std_dev, max, min};
	return results;
}

MaxInputFixedOutConnector::MaxInputFixedOutConnector(const std::string backend)
    : SNABBase(__func__, backend,
               {"Average number of sikes", "Standard deviation",
                "Maximum #spikes", "Minimum #spikes"},
               {"quality", "quality", "quality", "quality"}, {"", "", "", ""},
               {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes",
                "#input_neurons", "#ConnectionsPerInput"}),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &MaxInputFixedOutConnector::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	m_num_neurons = m_config_file["#neurons"];
	m_num_spikes = m_config_file["#spikes"];
	m_num_inp_neurons = m_config_file["#input_neurons"];

	// Get neuron neuron_parameters
	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(
		    10.0 + cypress::Real(i) * simulation_length / m_num_spikes);
	}
	m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
	    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times));
	std::cout << m_config_file["#ConnectionsPerInput"] << std::endl;
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::fixed_fan_out(size_t(m_config_file["#ConnectionsPerInput"]),
	                             cypress::Real(m_config_file["weight"])));
	return netw;
}
void MaxInputFixedOutConnector::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<cypress::Real> MaxInputFixedOutConnector::evaluate()
{
	// Gather the average #spikes of every neuron
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}

	// Calculate statistics
	cypress::Real max = *std::max_element(spikes.begin(), spikes.end());
	cypress::Real min = *std::min_element(spikes.begin(), spikes.end());
	cypress::Real avg = std::accumulate(spikes.begin(), spikes.end(), 0.0) /
	                    cypress::Real(spikes.size());
	cypress::Real std_dev = 0.0;
	std::for_each(spikes.begin(), spikes.end(), [&](const cypress::Real val) {
		std_dev += (val - avg) * (val - avg);
	});
	std_dev = std::sqrt(std_dev / cypress::Real(spikes.size() - 1));

	std::vector<cypress::Real> results = {avg, std_dev, max, min};
	return results;
}
}
