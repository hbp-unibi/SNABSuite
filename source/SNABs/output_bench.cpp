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
//#include <fstream>
#include <numeric>  // std::accumulate
#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank
#include <cypress/cypress.hpp>               // Neural network frontend

//#include "common/benchmark_base.hpp"
#include "common/neuron_parameters.hpp"
#include "output_bench.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
OutputFrequencySingleNeuron::OutputFrequencySingleNeuron(
    const std::string backend)
    : m_backend(backend), m_pop(m_netw, 0)
{
	m_config_file = read_config("OutputFrequencySingleNeuron", backend);
}

cypress::Network &OutputFrequencySingleNeuron::build_netw(
    cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	SpikingUtils::detect_type(neuron_type_str);
	m_config_file["neuron_params"];
	std::ofstream out;
	NeuronParameters neuron_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"], out);
	m_pop =
	    SpikingUtils::add_population(neuron_type_str, netw, neuron_params, 1);
	return netw;
}

void OutputFrequencySingleNeuron::run_netw(std::string backend,
                                           cypress::Network &netw)
{
	netw.logger().min_level(cypress::DEBUG, 0);
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(backend));
	netw.run(pwbackend, 100.0);
}

std::vector<cypress::Real> OutputFrequencySingleNeuron::evaluate()
{
	std::vector<cypress::Real> frequencies;
	auto spikes = m_pop.signals().data(0);
	if (spikes.size() != 0) {

		for (size_t i = 0; i < spikes.size() - 1; i++) {
			frequencies.push_back(cypress::Real(1.0) /
			                      (spikes[i + 1] - spikes[i]));
		}
	}
	else {
		frequencies.push_back(0.0);
	}
	cypress::Real max =
	    *std::max_element(frequencies.begin(), frequencies.end());
	cypress::Real min =
	    *std::min_element(frequencies.begin(), frequencies.end());
	cypress::Real avg =
	    std::accumulate(frequencies.begin(), frequencies.end(), 0.0) /
	    cypress::Real(frequencies.size());
	cypress::Real std_dev = 0.0;
	std::for_each(
	    frequencies.begin(), frequencies.end(),
	    [&](const cypress::Real val) { std_dev += (val - avg) * (val - avg); });
	std_dev = std::sqrt(std_dev / cypress::Real(frequencies.size() - 1));
	/*cypress::Json results = {
	    {{"type", "quality"},
	     {"name", "Average frequency"},
	     {"value", avg},
	     {"measure", "1/ms"}},
	    {{"type", "quality"},
	     {"name", "Standard deviation"},
	     {"value", std_dev},
	     {"measure", "1/ms"}},
	    {{"type", "quality"},
	     {"name", "Maximum"},
	     {"value", max},
	     {"measure", "1/ms"}},
	    {{"type", "quality"},
	     {"name", "Minimum"},
	     {"value", min},
	     {"measure", "1/ms"}},
	};*/
	std::vector<cypress::Real> results = {avg, std_dev, max, min};
	return results;
}
}
