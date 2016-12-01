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
//#include <algorithm>  // Minimal and Maximal element
//#include <numeric>    // std::accumulate
#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank
#include <cypress/cypress.hpp>               // Neural network frontend

#include "common/neuron_parameters.hpp"
#include "max_inter_neuron.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
SingleMaxFreqToGroup::SingleMaxFreqToGroup(const std::string backend)
    : BenchmarkBase(__func__, backend,
                    {"Average spike number deviation", "Standard deviation",
                     "Maximum", "Minimum"},
                    {"quality", "quality", "quality", "quality"},
                    {"1/ms", "1/ms", "1/ms", "1/ms"},
                    {"neuron_type", "neuron_params_max", "neuron_params_group",
                     "weight", "#neurons"}),
      m_pop_single(m_netw, 0),
      m_pop_group(m_netw, 0)
{
}
cypress::Network &SingleMaxFreqToGroup::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	// discard out put
	std::ofstream out;
	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"], out);
	m_group_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params_group"], out);

	// Create the single, alway spiking population
	m_pop_single = SpikingUtils::add_population(neuron_type_str, netw, params,
	                                            1, "spikes");
	// Create the group population
	m_pop_group =
	    SpikingUtils::add_population(neuron_type_str, netw, m_group_params,
	                                 m_config_file["#neurons"], "spikes");

	// Connnect the spiking neuron to the group
	netw.add_connection(
	    m_pop_single, m_pop_group,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}
void SingleMaxFreqToGroup::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length);
}

std::vector<cypress::Real> SingleMaxFreqToGroup::evaluate()
{
	// Reference spike count
	size_t spike_ref = m_pop_single[0].signals().data(0).size();
	if (spike_ref == 0) {
		std::cerr << "SNAB SingleMaxFreqToGroup was not configured correctly! "
		             "No spikes from single population!"
		          << std::endl;
		return std::vector<cypress::Real>();
	}

	std::vector<cypress::Real> spikes;
	for (size_t i = 0; i < size_t(m_config_file["#neurons"]); i++) {
		spikes.push_back(m_pop_group[i].signals().data(0).size());
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

	std::vector<cypress::Real> results = {avg - spike_ref, std_dev, max, min};
	return results;
}

GroupMaxFreqToGroup::GroupMaxFreqToGroup(const std::string backend)
    : BenchmarkBase(__func__, backend,
                    {"Average spike number deviation", "Standard deviation",
                     "Maximum", "Minimum"},
                    {"quality", "quality", "quality", "quality"},
                    {"1/ms", "1/ms", "1/ms", "1/ms"},
                    {"neuron_type", "neuron_params_max", "neuron_params_group",
                     "weight", "#neurons"}),
      m_pop_max(m_netw, 0),
      m_pop_retr(m_netw, 0)
{
}
cypress::Network &GroupMaxFreqToGroup::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	// discard out put
	std::ofstream out;
	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"], out);
	m_retr_params = NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params_group"], out);

	// Create the single, alway spiking population
	m_pop_max = SpikingUtils::add_population(
	    neuron_type_str, netw, params, m_config_file["#neurons"], "spikes");
	// Create the group population
	m_pop_retr =
	    SpikingUtils::add_population(neuron_type_str, netw, m_retr_params,
	                                 m_config_file["#neurons"], "spikes");

	// Connnect the spiking neuron to the group
	netw.add_connection(
	    m_pop_max, m_pop_retr,
	    Connector::one_to_one(cypress::Real(m_config_file["weight"])));
	return netw;
}
void GroupMaxFreqToGroup::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, simulation_length);
}

std::vector<cypress::Real> GroupMaxFreqToGroup::evaluate()
{
	// Reference spike count
	size_t spike_test = m_pop_max[0].signals().data(0).size();
	if (spike_test == 0) {
		std::cerr << "SNAB SingleMaxFreqToGroup was not configured correctly! "
		             "No spikes from single population!"
		          << std::endl;
		return std::vector<cypress::Real>();
	}

	std::vector<cypress::Real> spikes;
	std::vector<cypress::Real> spikes_ref;
	for (size_t i = 0; i < size_t(m_config_file["#neurons"]); i++) {
		spikes_ref.push_back(m_pop_max[i].signals().data(0).size());
		spikes.push_back(m_pop_retr[i].signals().data(0).size());
	}
	// Calculate statistics
	cypress::Real max = *std::max_element(spikes.begin(), spikes.end());
	cypress::Real min = *std::min_element(spikes.begin(), spikes.end());
	cypress::Real avg = std::accumulate(spikes.begin(), spikes.end(), 0.0) /
	                    cypress::Real(spikes.size());
	cypress::Real avg_ref =
	    std::accumulate(spikes_ref.begin(), spikes_ref.end(), 0.0) /
	    cypress::Real(spikes_ref.size());
	cypress::Real std_dev = 0.0;
	std::for_each(spikes.begin(), spikes.end(), [&](const cypress::Real val) {
		std_dev += (val - avg) * (val - avg);
	});
	std_dev = std::sqrt(std_dev / cypress::Real(spikes.size() - 1));

	std::vector<cypress::Real> results = {avg - avg_ref, std_dev, max, min};
	return results;
}
}
