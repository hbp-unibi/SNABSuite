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
#include <algorithm>  // Minimal and Maximal element
#include <limits>
#include <numeric>  // std::accumulate
#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank
#include <cypress/cypress.hpp>               // Neural network frontend

#include "common/neuron_parameters.hpp"
#include "output_bench.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
auto NaN = std::numeric_limits<cypress::Real>::quiet_NaN;

OutputFrequencySingleNeuron::OutputFrequencySingleNeuron(
    const std::string backend, size_t bench_index)
    : SNABBase(
          __func__, backend,
          {"Average frequency", "Standard deviation", "Maximum", "Minimum"},
          {"quality", "quality", "quality", "quality"},
          {"1/ms", "1/ms", "1/ms", "1/ms"}, {"neuron_type", "neuron_params"},
          bench_index),
      m_pop(m_netw, 0)
{
}

cypress::Network &OutputFrequencySingleNeuron::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	SpikingUtils::detect_type(neuron_type_str);

	// Get neuron neuron_parameters
	NeuronParameters neuron_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population
	m_pop =
	    SpikingUtils::add_population(neuron_type_str, netw, neuron_params, 1);
	return netw;
}

void OutputFrequencySingleNeuron::run_netw(cypress::Network &netw)
{
	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	try {
		netw.run(pwbackend, 150.0);
	}
	catch (const std::exception &exc) {
		std::cerr << exc.what();
		global_logger().fatal_error(
		    "SNABSuite",
		    "Wrong parameter setting or backend error! Simulation broke down");
	}
}

std::vector<cypress::Real> OutputFrequencySingleNeuron::evaluate()
{
	// Vector of frequencies
	std::vector<cypress::Real> frequencies;
	// Get spikes
	auto spikes = m_pop[0].signals().data(0);

	// Calculate frequencies
	if (spikes.size() > 1) {
		for (size_t i = 0; i < spikes.size() - 1; i++) {
			if (spikes[i] > 50) {
				// Workaround for a Bug in BrainScaleS
				if (spikes[i] == spikes[i + 1]) {
					continue;
				}
				frequencies.push_back(cypress::Real(1.0) /
				                      (spikes[i + 1] - spikes[i]));
			}
		}
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> temp({spikes});
	Utilities::write_vector2_to_csv(temp, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(frequencies, _debug_filename("freq.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("freq.csv"), m_backend, false,
	                          -10, "Frequencies");
#endif

	if (frequencies.size() == 0) {
		return std::vector<cypress::Real>({NaN(), NaN(), NaN(), NaN()});
	}

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(frequencies, min, max, avg, std_dev);
	return std::vector<cypress::Real>({avg, std_dev, max, min});
}

OutputFrequencySingleNeuron2::OutputFrequencySingleNeuron2(
    const std::string backend, size_t bench_index)
    : SNABBase(
          __func__, backend,
          {"Average frequency", "Standard deviation", "Maximum", "Minimum"},
          {"quality", "quality", "quality", "quality"},
          {"1/ms", "1/ms", "1/ms", "1/ms"},
          {"neuron_type", "neuron_params", "#neurons"}, bench_index),
      m_pop(m_netw, 0)
{
}

cypress::Network &OutputFrequencySingleNeuron2::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters neuron_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, neuron_params,
	                                     m_config_file["#neurons"], "");
	return netw;
}

void OutputFrequencySingleNeuron2::run_netw(cypress::Network &netw)
{
	// Reset the results
	m_spikes = std::vector<std::vector<cypress::Real>>();

	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));

	// Record Spikes only for ~16 neurons
	size_t step_size = size_t(float(m_pop.size() - 1) / 15.0);
	if (step_size == 0) {
		step_size = 1;
	}

	for (size_t i = 0; i < m_pop.size(); i += step_size) {
		for (size_t j = 0; j < m_pop.size(); j++) {
			m_pop[j].signals().record(0, i == j);
		}
		netw.run(pwbackend, 150.0);
		m_spikes.push_back(m_pop[i].signals().data(0));
	}
}

std::vector<cypress::Real> OutputFrequencySingleNeuron2::evaluate()
{
	if (m_spikes.size() == 0) {
		return std::vector<cypress::Real>({0, 0, 0, 0});
	}
	bool did_spike = false;
	std::vector<cypress::Real> mean_freq;
	for (size_t i = 0; i < m_spikes.size(); i++) {
		std::vector<cypress::Real> frequencies;
		for (int j = 0; j < int(m_spikes[i].size()) - 1; j++) {
            did_spike = true;
			if (m_spikes[i][j] > 50) {
				// Workaround for a Bug in BrainScaleS
				if (m_spikes[i][j + 1] == m_spikes[i][j]) {
					continue;
				}
				frequencies.push_back(cypress::Real(1.0) /
				                      (m_spikes[i][j + 1] - m_spikes[i][j]));
			}
		}
		if (frequencies.size() == 0) {
			mean_freq.push_back(0.0);
			continue;
		}
		cypress::Real avg =
		    std::accumulate(frequencies.begin(), frequencies.end(), 0.0) /
		    cypress::Real(frequencies.size());
		mean_freq.push_back(avg);
	}
#if SNAB_DEBUG
	// Write data to files
	Utilities::write_vector2_to_csv(m_spikes, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(mean_freq, _debug_filename("mean_freq.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("mean_freq.csv"), m_backend,
	                          false, -10, "Average Frequency");
#endif
	if (!did_spike) {
		return std::vector<cypress::Real>({NaN(), NaN(), NaN(), NaN()});
	}
	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(mean_freq, min, max, avg, std_dev);
	return std::vector<cypress::Real>({avg, std_dev, max, min});
}

OutputFrequencyMultipleNeurons::OutputFrequencyMultipleNeurons(
    const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend,
               {"Average frequency of neurons", "Standard deviation",
                "Maximum av frequency", "Minimum av frequency"},
               {"quality", "quality", "quality", "quality"},
               {"1/ms", "1/ms", "1/ms", "1/ms"},
               {"neuron_type", "neuron_params", "#neurons"}, bench_index),
      m_pop(m_netw, 0)
{
}

cypress::Network &OutputFrequencyMultipleNeurons::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];

	// Get neuron neuron_parameters
	NeuronParameters neuron_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);

	m_num_neurons = m_config_file["#neurons"];

	// Set up population
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, neuron_params,
	                                     m_num_neurons);
	return netw;
}

void OutputFrequencyMultipleNeurons::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, 150.0);
}

std::vector<cypress::Real> OutputFrequencyMultipleNeurons::evaluate()
{
	// Gather the average frequency of every neuron
	std::vector<cypress::Real> averages(m_num_neurons, -1);
    bool did_spike = false;
	for (size_t i = 0; i < m_num_neurons; i++) {
		// Vector of frequencies
		std::vector<cypress::Real> frequencies;

		// Get spikes
		auto spikes = m_pop[i].signals().data(0);
		// Calculate frequencies
		if (spikes.size() > 1) {
            did_spike = true;

			for (size_t i = 0; i < spikes.size() - 1; i++) {
				if (spikes[i] > 50) {
					// Workaround for a Bug in BrainScaleS
					if (spikes[i] == spikes[i + 1]) {
						continue;
					}
					frequencies.push_back(cypress::Real(1.0) /
					                      (spikes[i + 1] - spikes[i]));
				}
			}
		}
		if (frequencies.size() == 0) {
			frequencies.push_back(0.0);
		}

		// Calculate average
		averages[i] =
		    std::accumulate(frequencies.begin(), frequencies.end(), 0.0) /
		    cypress::Real(frequencies.size());
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes2;
	for (size_t i = 0; i < m_num_neurons; i++) {
		spikes2.push_back(m_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes2, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(averages, _debug_filename("averages.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("averages.csv"), m_backend, false,
	                          -10, "Averages");
#endif
	if (!did_spike) {
		return std::vector<cypress::Real>({NaN(), NaN(), NaN(), NaN()});
	}

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(averages, min, max, avg, std_dev);
	return std::vector<cypress::Real>({avg, std_dev, max, min});
}
}
