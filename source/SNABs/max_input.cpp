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
#include <cypress/backend/power/power.hpp>  // Control of power via netw
#include <cypress/cypress.hpp>              // Neural network frontend
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "max_input.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;

MaxInputOneToOne::MaxInputOneToOne(const std::string backend,
                                   size_t bench_index)
    : SNABBase(
          __func__, backend, {"Average number of spikes"}, {"quality"},
          {"#spikes"}, {""},
          {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes"},
          bench_index),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &MaxInputOneToOne::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	m_num_neurons = m_config_file["#neurons"];
	m_num_spikes = m_config_file["#spikes"];
	if (m_config_file.find("runtime") != m_config_file.end()) {
		simulation_length = m_config_file["runtime"].get<Real>();
	}
	bool record_source = false;
	if (m_config_file.find("record_spikes") != m_config_file.end()) {
		record_source = m_config_file["record_spikes"].get<bool>();
	}

	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(10.0 + cypress::Real(i) * simulation_length /
		                                    m_num_spikes);
	}
	if (!record_source) {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_neurons, SpikeSourceArrayParameters(spike_times));
	}
	else {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_neurons, SpikeSourceArrayParameters(spike_times),
		    SpikeSourceArraySignals().record_spikes());
	}
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::one_to_one(cypress::Real(m_config_file["weight"])));
	return netw;
}
void MaxInputOneToOne::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<std::array<cypress::Real, 4>> MaxInputOneToOne::evaluate()
{
	// Gather the average #spikes of every neuron
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}
#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes2;
	for (size_t i = 0; i < m_pop.size(); i++) {
		spikes2.push_back(m_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes2, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(spikes, _debug_filename("num_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("num_spikes.csv"), m_backend,
	                          false, -10, "'Number of Spikes per Neuron'");
#endif

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(spikes, min, max, avg, std_dev);
	return {std::array<cypress::Real, 4>({avg, std_dev, min, max})};
}

MaxInputAllToAll::MaxInputAllToAll(const std::string backend,
                                   size_t bench_index)
    : SNABBase(__func__, backend, {"Average number of spikes"}, {"quality"},
               {"#spikes"}, {""},
               {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes",
                "#input_neurons"},
               bench_index),
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
	if (m_config_file.find("runtime") != m_config_file.end()) {
		simulation_length = m_config_file["runtime"].get<Real>();
	}
	bool record_source = false;
	if (m_config_file.find("record_spikes") != m_config_file.end()) {
		record_source = m_config_file["record_spikes"].get<bool>();
	}

	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(10.0 + cypress::Real(i) * simulation_length /
		                                    m_num_spikes);
	}
	if (!record_source) {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times));
	}
	else {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times),
		    SpikeSourceArraySignals().record_spikes());
	}
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}
void MaxInputAllToAll::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<std::array<cypress::Real, 4>> MaxInputAllToAll::evaluate()
{
	// Gather the average #spikes of every neuron
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes2;
	for (size_t i = 0; i < m_pop.size(); i++) {
		spikes2.push_back(m_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes2, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(spikes, _debug_filename("num_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("num_spikes.csv"), m_backend,
	                          false, -10, "'Number of Spikes per Neuron'");
#endif

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(spikes, min, max, avg, std_dev);
	return {std::array<cypress::Real, 4>({avg, std_dev, min, max})};
}

MaxInputFixedOutConnector::MaxInputFixedOutConnector(const std::string backend,
                                                     size_t bench_index)
    : SNABBase(__func__, backend, {"Average number of spikes"}, {"quality"},
               {"#spikes"}, {""},
               {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes",
                "#input_neurons", "#ConnectionsPerInput"},
               bench_index),
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
	if (m_config_file.find("runtime") != m_config_file.end()) {
		simulation_length = m_config_file["runtime"].get<Real>();
	}
	bool record_source = false;
	if (m_config_file.find("record_spikes") != m_config_file.end()) {
		record_source = m_config_file["record_spikes"].get<bool>();
	}


	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(10.0 + cypress::Real(i) * simulation_length /
		                                    m_num_spikes);
	}
	if (!record_source) {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times));
	}
	else {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times),
		    SpikeSourceArraySignals().record_spikes());
	}

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

	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length + 50.0);
}

std::vector<std::array<cypress::Real, 4>> MaxInputFixedOutConnector::evaluate()
{
	// Gather the average #spikes of every neuron, init with -1
	std::vector<cypress::Real> spikes(m_num_neurons, -1);
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Get #spikes
		spikes[i] = m_pop[i].signals().data(0).size();
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes2;
	for (size_t i = 0; i < m_pop.size(); i++) {
		spikes2.push_back(m_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes2, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(spikes, _debug_filename("num_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("num_spikes.csv"), m_backend,
	                          false, -10, "'Number of Spikes per Neuron'");
#endif

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(spikes, min, max, avg, std_dev);
	return {std::array<cypress::Real, 4>({avg, std_dev, min, max})};
}

MaxInputFixedOutConnector::MaxInputFixedOutConnector(
    std::string name, std::string backend,
    std::initializer_list<std::string> indicator_names,
    std::initializer_list<std::string> indicator_types,
    std::initializer_list<std::string> indicator_measures,
    std::initializer_list<std::string> indicator_units,
    std::initializer_list<std::string> required_parameters, size_t bench_index)
    : SNABBase(name, backend, indicator_names, indicator_types,
               indicator_measures, indicator_units, required_parameters,
               bench_index),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}

MaxInputFixedInConnector::MaxInputFixedInConnector(const std::string backend,
                                                   size_t bench_index)
    : MaxInputFixedOutConnector(
          __func__, backend, {"Average number of spikes"}, {"quality"},
          {"#spikes"}, {""},
          {"neuron_type", "neuron_params", "weight", "#neurons", "#spikes",
           "#input_neurons", "#ConnectionsPerOutput"},
          bench_index)
{
}
cypress::Network &MaxInputFixedInConnector::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	m_num_neurons = m_config_file["#neurons"];
	m_num_spikes = m_config_file["#spikes"];
	m_num_inp_neurons = m_config_file["#input_neurons"];
	if (m_config_file.find("runtime") != m_config_file.end()) {
		simulation_length = m_config_file["runtime"].get<Real>();
	}
	bool record_source = false;
	if (m_config_file.find("record_spikes") != m_config_file.end()) {
		record_source = m_config_file["record_spikes"].get<bool>();
	}


	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     m_num_neurons, "spikes");
	std::vector<cypress::Real> spike_times;
	for (size_t i = 0; i < m_num_spikes; i++) {
		spike_times.emplace_back(10.0 + cypress::Real(i) * simulation_length /
		                                    m_num_spikes);
	}
	if (!record_source) {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times));
	}
	else {
		m_pop_source = netw.create_population<cypress::SpikeSourceArray>(
		    m_num_inp_neurons, SpikeSourceArrayParameters(spike_times),
		    SpikeSourceArraySignals().record_spikes());
	}

	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::fixed_fan_in(size_t(m_config_file["#ConnectionsPerOutput"]),
	                            cypress::Real(m_config_file["weight"])));
	return netw;
}

}  // namespace SNAB
