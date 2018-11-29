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
#include <cypress/cypress.hpp>               // Neural network frontend

#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank

#include "common/neuron_parameters.hpp"
#include "max_inter_neuron.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;

SingleMaxFreqToGroup::SingleMaxFreqToGroup(const std::string backend,
                                           size_t bench_index)
    : SNABBase(__func__, backend,
               {"Average spike number deviation", "Standard deviation",
                "Maximum", "Minimum"},
               {"quality", "quality", "quality", "quality"},
               {"1/ms", "1/ms", "1/ms", "1/ms"},
               {"neuron_type", "neuron_params_max", "neuron_params_retr",
                "weight", "#neurons"},
               bench_index),
      m_pop_single(m_netw, 0),
      m_pop_group(m_netw, 0)
{
}
cypress::Network &SingleMaxFreqToGroup::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"]);
	m_group_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params_retr"]);

	// Create the single, always spiking population
	m_pop_single = SpikingUtils::add_population(neuron_type_str, netw, params,
	                                            1, "spikes");
	// Create the group population
	m_pop_group =
	    SpikingUtils::add_population(neuron_type_str, netw, m_group_params,
	                                 m_config_file["#neurons"], "spikes");

	// Connect the spiking neuron to the group
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
	netw.run(pwbackend, m_simulation_length);
}

std::vector<cypress::Real> SingleMaxFreqToGroup::evaluate()
{
	// Reference spike count
	size_t spike_ref = SpikingUtils::calc_num_spikes(
	    m_pop_single[0].signals().data(0), m_start_time);
    bool valid = true;
	if (spike_ref < (m_simulation_length - m_start_time) /
	                    10) {  // less than a spike every 10 ms
		global_logger().error("SNABSuite", "SNAB SingleMaxFreqToGroup was probably not configured "
		             "correctly! "
		             "No spikes from single population!");
		valid = false;
	}
	std::vector<int> num_spikes;
	for (size_t i = 0; i < m_pop_group.size(); i++) {
		num_spikes.push_back(SpikingUtils::calc_num_spikes(
		    m_pop_group[i].signals().data(0), m_start_time));
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop_group.size(); i++) {
		spikes.push_back(m_pop_group[i].signals().data(0));
	}
	std::vector<std::vector<cypress::Real>> ref_spikes(
	    {m_pop_single[0].signals().data(0)});
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(num_spikes,
	                               _debug_filename("num_spikes.csv"));
	Utilities::write_vector2_to_csv(ref_spikes,
	                                _debug_filename("ref_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("ref_spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("num_spikes.csv"), m_backend,
	                          false, -10, "'Number of Spikes'");
#endif
    
    if(!valid){
        return std::vector<cypress::Real>({NaN(), NaN(), NaN(), NaN()});
    }

	// Calculate statistics
	cypress::Real avg, std_dev;
	int min, max;
	Utilities::calculate_statistics<int>(num_spikes, min, max, avg, std_dev);
	return std::vector<cypress::Real>({avg - cypress::Real(spike_ref), std_dev,
	                                   max - cypress::Real(spike_ref),
	                                   min - cypress::Real(spike_ref)});
}

GroupMaxFreqToGroup::GroupMaxFreqToGroup(const std::string backend,
                                         size_t bench_index)
    : SNABBase(__func__, backend,
               {"Average number of spikes", "Standard deviation",
                "Maximum", "Minimum"},
               {"quality", "quality", "quality", "quality"},
               {"", "", "", ""},
               {"neuron_type", "neuron_params_max", "neuron_params_retr",
                "weight", "#neurons"},
               bench_index),
      m_pop_max(m_netw, 0),
      m_pop_retr(m_netw, 0)
{
}
cypress::Network &GroupMaxFreqToGroup::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"]);
	m_retr_params = NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params_retr"]);

	// Create the always spiking population
	m_pop_max = SpikingUtils::add_population(
	    neuron_type_str, netw, params, m_config_file["#neurons"], "");
	// Create the group population
	m_pop_retr =
	    SpikingUtils::add_population(neuron_type_str, netw, m_retr_params,
	                                 m_config_file["#neurons"], "spikes");

	// Connect the spiking neurons to the group
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
	netw.run(pwbackend, m_simulation_length);
}

std::vector<cypress::Real> GroupMaxFreqToGroup::evaluate()
{
    std::vector<size_t> num_spikes;
	for (size_t i = 0; i < m_pop_retr.size(); i++) {
		num_spikes.push_back(SpikingUtils::calc_num_spikes(
		    m_pop_retr[i].signals().data(0), m_start_time));
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop_retr.size(); i++) {
		spikes.push_back(m_pop_retr[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("spikes.csv"));

	Utilities::write_vector_to_csv(num_spikes,
	                               _debug_filename("num_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("num_spikes.csv"), m_backend,
	                          false, -10, "'Number of Spikes (Target)'");
#endif

	// Calculate statistics
	cypress::Real avg, std_dev;
    size_t min, max;
	Utilities::calculate_statistics<size_t>(num_spikes, min, max, avg, std_dev);

	return std::vector<cypress::Real>({avg, std_dev, cypress::Real(max), cypress::Real(min)});
}

GroupMaxFreqToGroup::GroupMaxFreqToGroup(
    std::string name, std::string backend,
    std::initializer_list<std::string> indicator_names,
    std::initializer_list<std::string> indicator_types,
    std::initializer_list<std::string> indicator_measures,
    std::initializer_list<std::string> required_parameters, size_t bench_index)
    : SNABBase(name, backend, indicator_names, indicator_types,
               indicator_measures, required_parameters, bench_index),
      m_pop_max(m_netw, 0),
      m_pop_retr(m_netw, 0)
{
}

GroupMaxFreqToGroupAllToAll::GroupMaxFreqToGroupAllToAll(
    const std::string backend, size_t bench_index)
    : GroupMaxFreqToGroup(__func__, backend,
               {"Average number of spikes", "Standard deviation",
                "Maximum", "Minimum"},
               {"quality", "quality", "quality", "quality"},
               {"", "", "", ""},
               {"neuron_type", "neuron_params_max", "neuron_params_retr",
                "weight", "#neurons_max", "#neurons_retr"},
               bench_index)
{
}
cypress::Network &GroupMaxFreqToGroupAllToAll::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"]);
	m_retr_params = NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params_retr"]);

	// Create the single always spiking population
	m_pop_max = SpikingUtils::add_population(
	    neuron_type_str, netw, params, m_config_file["#neurons_max"], "");
	// Create the group population
	m_pop_retr =
	    SpikingUtils::add_population(neuron_type_str, netw, m_retr_params,
	                                 m_config_file["#neurons_retr"], "spikes");

	// Connect the spiking neuron to the group
	netw.add_connection(
	    m_pop_max, m_pop_retr,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}


GroupMaxFreqToGroupProb::GroupMaxFreqToGroupProb(
    const std::string backend, size_t bench_index)
    : GroupMaxFreqToGroup(__func__, backend,
               {"Average number of spikes", "Standard deviation",
                "Maximum", "Minimum"},
               {"quality", "quality", "quality", "quality"},
               {"", "", "", ""},
               {"neuron_type", "neuron_params_max", "neuron_params_retr",
                "weight", "#neurons_max", "#neurons_retr", "probability"},
               bench_index)
{
}
cypress::Network &GroupMaxFreqToGroupProb::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters params(SpikingUtils::detect_type(neuron_type_str),
	                        m_config_file["neuron_params_max"]);
	m_retr_params = NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params_retr"]);

	// Create the always spiking population
	m_pop_max = SpikingUtils::add_population(
	    neuron_type_str, netw, params, m_config_file["#neurons_max"], "");
	// Create the group population
	m_pop_retr =
	    SpikingUtils::add_population(neuron_type_str, netw, m_retr_params,
	                                 m_config_file["#neurons_retr"], "spikes");

	// Connect the spiking neuron to the group
	netw.add_connection(
	    m_pop_max, m_pop_retr,
	    Connector::random(cypress::Real(m_config_file["weight"]), 0.0, cypress::Real(m_config_file["probability"])));
	return netw;
}
}  // namespace SNAB
