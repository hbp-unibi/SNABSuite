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

#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank
#include <cypress/cypress.hpp>               // Neural network frontend

#include "common/neuron_parameters.hpp"
#include "refractory_period.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
RefractoryPeriod::RefractoryPeriod(const std::string backend,
                                   size_t bench_index)
    : SNABBase(__func__, backend, {"Average deviation from refractory period",
                                   "Standard deviation"},
               {"quality", "quality"}, {"ms", "ms"},
               {"neuron_type", "neuron_params", "weight"}, bench_index),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &RefractoryPeriod::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	auto &neuro_type = SpikingUtils::detect_type(neuron_type_str);

	// Get neuron neuron_parameters
	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(neuron_type_str),
	                     m_config_file["neuron_params"]);
	// Set up population, record voltage
	m_pop = SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                     1, "v");
	// Record spikes
	m_pop.signals().record(neuro_type.signal_index("spikes").value());

	// 10 input neurons with spike times
	m_pop_source = netw.create_population<SpikeSourceArray>(10);
	for (auto i : m_pop_source) {
		i.parameters().spike_times({10, 30, 50, 70, 90, 110, 130});
	}
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}

void RefractoryPeriod::run_netw(cypress::Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(cypress::DEBUG, 0);

	if (m_backend == "spikey") {
		m_backend.append("={\"calibIcb\": 1}");
	}

	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend, 150.0);
}

std::vector<cypress::Real> RefractoryPeriod::evaluate()
{
	auto voltage = m_pop[0].signals().data(1);
	auto spike_time = m_pop[0].signals().data(0);
	std::vector<cypress::Real> starts;
	std::vector<cypress::Real> ends;
	cypress::Real v_reset = m_neuro_params.get("v_reset");
	cypress::Real ref_per = m_neuro_params.get("tau_refrac");
	cypress::Real tolerance = 1.0;  // in mV
	bool started = false;

	// Check if backend spiked at all
	if (spike_time.size() == 0) {
		std::cerr
		    << "Refractory period could not be measured! Adjust parameters."
		    << std::endl;
		return std::vector<cypress::Real>();
	}

	// Gather start and end points of the refractory periods by running through
	// the voltage trace
	for (size_t i = 0; i < voltage.rows(); i++) {
		if (!started && voltage(i, 1) < v_reset + tolerance) {
			started = true;
			starts.emplace_back(voltage(i, 0));
		}
		else if (started && voltage(i, 1) > v_reset + tolerance) {
			ends.emplace_back(voltage(i - 1, 0));
			started = false;
			i += 10;
		}
	}

	// Calculate periods
	std::vector<cypress::Real> diffs;
	for (size_t i = 0; i < ends.size(); i++) {
		diffs.emplace_back(ends[i] - starts[i] - ref_per);
	}

#if SNAB_DEBUG
	std::vector<std::vector<cypress::Real>> time_voltage;
	for (size_t i = 0; i < voltage.rows(); i++) {
		time_voltage.push_back(
		    std::vector<cypress::Real>{voltage(i, 0), voltage(i, 1)});
	}
	Utilities::write_vector2_to_csv(time_voltage,
	                                "RefractoryPeriod_voltage.csv");
	Utilities::write_vector_to_csv(spike_time,
	                               "RefractoryPeriod_spike_time.csv");
	Utilities::write_vector_to_csv(diffs, "RefractoryPeriod_periods.csv");
#endif

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(diffs, min, max, avg, std_dev);
	return std::vector<cypress::Real>({avg, std_dev});
}
}
