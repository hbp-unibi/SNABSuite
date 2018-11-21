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
#include <cypress/cypress.hpp>  // Neural network frontend

#include <memory>
#include <random>
#include <string>
#include <vector>

#include <cypress/backend/power/power.hpp>
#include "activation_curve.hpp"
#include "common/neuron_parameters.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using cypress::Real;
WeightDependentActivation::WeightDependentActivation(const std::string backend,
                                                     size_t bench_index)
    : SNABBase(__func__, backend,
               {"Average spike deviation", "Average standard deviation",
                "Maximum deviation", "Minimum deviation"},
               {"quality", "quality", "quality", "quality"}, {"", "", "", ""},
               {"neuron_type", "neuron_params", "weight_min", "weight_max",
                "step_size", "#neurons", "isi", "expected_output"},
               bench_index),
      m_pop(m_netw, 0)
{
	Real step_size = m_config_file["step_size"];
	m_num_steps = size_t((Real(m_config_file["weight_max"]) -
	                      Real(m_config_file["weight_min"])) /
	                     step_size);
	if (m_config_file["expected_output"].size() != m_num_steps) {
		global_logger().warn("SNABSuite",
		                     "WeightDependentActivation: size of expected "
		                     "output does not match with configuration!");
		m_valid = false;
	}
}

WeightDependentActivation::WeightDependentActivation(
    std::string name, std::string backend,
    std::initializer_list<std::string> indicator_names,
    std::initializer_list<std::string> indicator_types,
    std::initializer_list<std::string> indicator_measures,
    std::initializer_list<std::string> required_parameters, size_t bench_index)
    : SNABBase(name, backend, indicator_names, indicator_types,
               indicator_measures, required_parameters, bench_index),
      m_pop(m_netw, 0)
{
}

cypress::Network &WeightDependentActivation::build_netw(cypress::Network &netw)
{
	// Get neuron neuron_parameters
	auto neuro_params = NeuronParameters(
	    SpikingUtils::detect_type(m_config_file["neuron_type"]),
	    m_config_file["neuron_params"]);
	// Set up population, record spikes
	m_pop = SpikingUtils::add_population(m_config_file["neuron_type"], netw,
	                                     neuro_params,
	                                     m_config_file["#neurons"], "spikes");

	Real step_size = m_config_file["step_size"];
	m_num_steps = size_t((Real(m_config_file["weight_max"]) -
	                      Real(m_config_file["weight_min"])) /
	                     step_size);

	// Create source populations
	for (size_t i = 0; i < m_num_steps; i++) {
		m_pop_source.push_back(
		    netw.create_population<cypress::SpikeSourceArray>(
		        m_config_file["#neurons"]));
	}

	for (size_t step = 0; step < m_num_steps; step++) {
		for (size_t i = 0; i < m_pop_source[step].size(); i++) {
			// Calculate the spike time in dependence of the weight, and the
			// neuron
			Real spike = m_offset +
			             Real(m_config_file["isi"]) /
			                 Real(num_neurons_per_cycle) *
			                 Real(i % num_neurons_per_cycle) +
			             Real(step) * Real(m_config_file["isi"]);
			m_pop_source[step][i].parameters().spike_times({spike});
		}

		netw.add_connection(
		    m_pop_source[step], m_pop,
		    Connector::one_to_one(Real(m_config_file["weight_min"]) +
		                              step * Real(m_config_file["step_size"]),
		                          1.0));
	}
	return netw;
}

void WeightDependentActivation::run_netw(cypress::Network &netw)
{
	// PowerManagementBackend to use trigger network plug
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend,
	         m_offset + (m_num_steps + 2) * Real(m_config_file["isi"]));
}

std::vector<std::vector<Real>> WeightDependentActivation::binned_spike_counts()
{
	std::vector<std::vector<Real>> binned_spike_counts;
	for (size_t i = 0; i < m_pop.size(); i++) {
		Real start = m_offset + Real(m_config_file["isi"]) /
		                            Real(num_neurons_per_cycle) *
		                            Real(i % num_neurons_per_cycle);
		Real stop = start + m_num_steps * Real(m_config_file["isi"]);

		binned_spike_counts.push_back(SpikingUtils::spike_time_binning<Real>(
		    start, stop, m_num_steps, m_pop[i].signals().data(0)));
	}
	return binned_spike_counts;
}

namespace {
Real round_2_dec(Real val)
{
	return std::llround(val * 100.0) / 100.0;
}
}  // namespace
std::vector<cypress::Real> WeightDependentActivation::evaluate()
{
	// Gather the #spikes of every neuron
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop.size(); i++) {
		// Get spikes
		spikes.push_back(m_pop[i].signals().data(0));
	}

	// Do spike binning
	std::vector<std::vector<Real>> binned_spike_counts =
	    this->binned_spike_counts();

	// Calculate average bin values
	std::vector<Real> max(m_num_steps, 0), min(m_num_steps, 0),
	    avg(m_num_steps, 0), std_dev(m_num_steps, 0);

	for (size_t i = 0; i < m_num_steps; i++) {
		std::vector<Real> bins(m_pop.size(), 0);
		for (size_t j = 0; j < m_pop.size(); j++) {
			bins[j] = Real(binned_spike_counts[j][i]);
		}
		Utilities::calculate_statistics(bins, min[i], max[i], avg[i],
		                                std_dev[i]);
	}

#if SNAB_DEBUG
	// Write all possible data to file
	Utilities::write_vector2_to_csv(binned_spike_counts,
	                                _debug_filename("spike_counts.csv"));
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(avg, _debug_filename("avg.csv"));
	Utilities::write_vector_to_csv(min, _debug_filename("min.csv"));
	Utilities::write_vector_to_csv(max, _debug_filename("max.csv"));
	Utilities::write_vector_to_csv(std_dev, _debug_filename("std_dev.csv"));

	// Prepare 1D plots
	std::vector<std::vector<Real>> plot_data(m_num_steps,
	                                         std::vector<Real>(5, 0));
	for (size_t i = 0; i < m_num_steps; i++) {
		plot_data[i][0] = Real(m_config_file["weight_min"]) +
		                  i * Real(m_config_file["step_size"]);
		plot_data[i][1] = round_2_dec(avg[i]);
		plot_data[i][2] = round_2_dec(std_dev[i]);
		plot_data[i][3] = round_2_dec(min[i]);
		plot_data[i][4] = round_2_dec(max[i]);
	}
	if (m_snab_name == "WeightDependentActivation") {
		Utilities::write_vector2_to_csv(
		    plot_data, _debug_filename("plot.csv"),
		    "#weight,Average number of spikes,Standard deviation,Minimum "
		    "#spikes,Maximum #spikes,");
	}
	else {
		Utilities::write_vector2_to_csv(
		    plot_data, _debug_filename("plot.csv"),
		    "#weight,Average_Frequency_of_neurons,Standard deviation,Minimum "
		    "freq,Maximum freq,");
	}

	for (size_t i = 0; i < m_num_steps; i++) {
		plot_data[i][1] = round_2_dec(avg[i] - Real(m_config_file["expected_output"][i]));
	}

	if (m_snab_name == "WeightDependentActivation") {
		Utilities::write_vector2_to_csv(
		    plot_data, _debug_filename("plot_dev.csv"),
		    "#weight,Average deviation,Standard deviation,Minimum "
		    "#spikes,Maximum #spikes,");
	}
	else {
		Utilities::write_vector2_to_csv(
		    plot_data, _debug_filename("plot_dev.csv"),
		    "#weight,Average_freq_deviation,Standard deviation,Minimum "
		    "freq,Maximum freq,");
	}

	// Trigger plots
	Utilities::plot_1d_curve(_debug_filename("plot.csv"), m_backend, 0, 1, 2);
	Utilities::plot_1d_curve(_debug_filename("plot_dev.csv"), m_backend, 0, 1,
	                         2);
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
#endif

	// Calculate absolute max and min values
	Real max_max = 0.0, min_min = 0.0;
	for (size_t i = 0; i < m_num_steps; i++) {
		avg[i] = avg[i] - Real(m_config_file["expected_output"][i]);
		if (max[i] - Real(m_config_file["expected_output"][i]) > max_max) {
			max_max = max[i] - Real(m_config_file["expected_output"][i]);
		}
		if (min[i] - Real(m_config_file["expected_output"][i]) < min_min) {
			min_min = min[i] - Real(m_config_file["expected_output"][i]);
		}
	}

	// Calculate statistics
	cypress::Real avg_avg, avg_std_dev, dummy, dummy2;
	Utilities::calculate_statistics(avg, dummy2, dummy, avg_avg, dummy2);
	Utilities::calculate_statistics(std_dev, dummy2, dummy, avg_std_dev,
	                                dummy2);
	return {round_2_dec(avg_avg), round_2_dec(avg_std_dev),
	        round_2_dec(max_max), round_2_dec(min_min)};
}

RateBasedWeightDependentActivation::RateBasedWeightDependentActivation(
    const std::string backend, size_t bench_index)
    : WeightDependentActivation(
          __func__, backend,
          {"Average frequency deviation", "Average standard deviation",
           "Maximum deviation", "Minimum deviation"},
          {"quality", "quality", "quality", "quality"}, {"", "", "", ""},
          {"neuron_type", "neuron_params", "weight_min", "weight_max",
           "step_size", "#neurons", "presentation_time", "rate",
           "expected_output"},
          bench_index)
{
	Real step_size = m_config_file["step_size"];
	m_num_steps = size_t((Real(m_config_file["weight_max"]) -
	                      Real(m_config_file["weight_min"])) /
	                     step_size);
	if (m_config_file["expected_output"].size() != m_num_steps) {
		global_logger().warn(
		    "SNABSuite",
		    "RateBasedWeightDependentActivation: size of expected "
		    "output does not match with configuration!");
		m_valid = false;
	}
}

namespace {
std::vector<Real> spike_rate(Real t_begin, Real t_end, Real freq)
{
	Real intervall = 1000.0 / freq;  // convert freq from Hz
	Real num_spikes = (t_end - t_begin) * freq / 1000.0;

	std::vector<Real> spikes(num_spikes, 0);
	for (size_t i = 0; i < num_spikes; i++) {
		spikes[i] = t_begin + i * intervall;
	}
	return spikes;
}
}  // namespace

cypress::Network &RateBasedWeightDependentActivation::build_netw(
    cypress::Network &netw)
{
	// Get neuron neuron_parameters
	auto neuro_params = NeuronParameters(
	    SpikingUtils::detect_type(m_config_file["neuron_type"]),
	    m_config_file["neuron_params"]);
	// Set up population, record spikes
	m_pop = SpikingUtils::add_population(m_config_file["neuron_type"], netw,
	                                     neuro_params,
	                                     m_config_file["#neurons"], "spikes");

	Real step_size = m_config_file["step_size"];
	m_num_steps = size_t((Real(m_config_file["weight_max"]) -
	                      Real(m_config_file["weight_min"])) /
	                     step_size);

	// Create source populations
	for (size_t i = 0; i < m_num_steps; i++) {
		m_pop_source.push_back(
		    netw.create_population<cypress::SpikeSourceArray>(
		        m_config_file["#neurons"]));
	}

	Real presentation_time = Real(m_config_file["presentation_time"]);
	for (size_t step = 0; step < m_num_steps; step++) {
		for (size_t i = 0; i < m_pop_source[step].size(); i++) {
			// Calculate the spikes time in dependence of the weight, and the
			// neuron
			Real start = m_offset +
			             step * num_neurons_per_cycle * presentation_time +
			             Real(i % num_neurons_per_cycle) * presentation_time;

			m_pop_source[step][i].parameters().spike_times(spike_rate(
			    start, start + presentation_time, Real(m_config_file["rate"])));
		}

		netw.add_connection(
		    m_pop_source[step], m_pop,
		    Connector::one_to_one(Real(m_config_file["weight_min"]) +
		                              step * Real(m_config_file["step_size"]),
		                          1.0));
	}
	return netw;
}
namespace {
std::vector<Real> get_spikes_in_intervall(const Real start, const Real stop,
                                          const std::vector<Real> &spikes)
{
	std::vector<Real> res;
	for (auto i : spikes) {
		if (start < i && i < stop) {
			res.push_back(i);
		}
		if (i > stop) {
			break;
		}
	}
	return res;
}

std::vector<Real> spike_to_freq(const std::vector<Real> &spikes)
{
	std::vector<Real> freq;
	if (spikes.size() < 2) {
		return freq;
	}
	for (size_t i = 0; i < spikes.size() - 1; i++) {
		freq.push_back(1.0 / (spikes[i + 1] - spikes[i]));
	}
	return freq;
}
}  // namespace
std::vector<std::vector<Real>>
RateBasedWeightDependentActivation::binned_spike_counts()
{
	std::vector<std::vector<Real>> binned_spike_freq(
	    m_pop.size(), std::vector<Real>(m_num_steps, 0.0));

	Real presentation_time = Real(m_config_file["presentation_time"]);
	for (size_t step = 0; step < m_num_steps; step++) {
		for (size_t i = 0; i < m_pop_source[step].size(); i++) {
			Real start = m_offset +
			             step * num_neurons_per_cycle * presentation_time +
			             Real(i % num_neurons_per_cycle) * presentation_time;
			Real stop = start + presentation_time;
			start = stop - fraction_pres_time * presentation_time;
			auto temp = spike_to_freq(get_spikes_in_intervall(
			    start, stop, m_pop[i].signals().data(0)));
			if (temp.size() > 0) {
				binned_spike_freq[i][step] =
				    1000.0 * std::accumulate(temp.begin(), temp.end(), 0.0) /
				    Real(temp.size());
			}
		}
	}
	return binned_spike_freq;
}

void RateBasedWeightDependentActivation::run_netw(cypress::Network &netw)
{
	// PowerManagementBackend to use trigger network plug
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend,
	         m_offset + num_neurons_per_cycle * (m_num_steps + 1) *
	                        Real(m_config_file["presentation_time"]));
}

}  // namespace SNAB
