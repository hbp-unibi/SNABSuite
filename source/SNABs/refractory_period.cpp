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
#include <cypress/cypress.hpp>               // Neural network frontend

#include <limits>
#include <string>
#include <vector>

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank

#include "common/neuron_parameters.hpp"
#include "refractory_period.hpp"
#include "util/read_json.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
RefractoryPeriod::RefractoryPeriod(const std::string backend,
                                   size_t bench_index)
    : SNABBase(
          __func__, backend,
          {"Average deviation from refractory period"},
          {"quality"},{"deviation"}, {"ms"},
          {"neuron_type", "neuron_params", "weight", "tolerance"}, bench_index),
      m_pop(m_netw, 0),
      m_pop_source(cypress::PopulationBase(m_netw, 0))
{
}
cypress::Network &RefractoryPeriod::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	auto &neuro_type = SpikingUtils::detect_type(neuron_type_str);
	m_tolerance = m_config_file["tolerance"];

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
		i.parameters().spike_times(
		    {10, 30, 50, 70, 90, 110, 130, 150, 170, 190});
	}
	netw.add_connection(
	    m_pop_source, m_pop,
	    Connector::all_to_all(cypress::Real(m_config_file["weight"])));
	return netw;
}

void RefractoryPeriod::run_netw(cypress::Network &netw)
{
	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	SpikingUtils::rerun_fixed_number_trials(netw, pwbackend, 250, 3);
}

std::vector<std::array<cypress::Real, 4>> RefractoryPeriod::evaluate()
{
	auto voltage = m_pop[0].signals().data(1);
	auto spike_time = m_pop[0].signals().data(0);
	std::vector<cypress::Real> starts;
	std::vector<cypress::Real> ends;
	// Calculations are done with tolerance
	cypress::Real v_reset = m_neuro_params.get("v_reset") + m_tolerance;
	cypress::Real ref_per = m_neuro_params.get("tau_refrac");
	bool started = false;

	std::vector<cypress::Real> diffs;

	// Check if backend spiked at all
	if (spike_time.size() != 0) {
		// Gather start and end points of the refractory periods by running
		// through the voltage trace
		for (size_t i = 0; i < voltage.rows(); i++) {
			if (!started && voltage(i, 1) < v_reset) {
				started = true;
				starts.emplace_back(voltage(i, 0));
			}
			else if (started && voltage(i, 1) > v_reset) {
				// Compatibility workaround for analog hardware:
				// When outside of refractory domain, check wheter this is
				// caused by fluctuations > m_tolerance
				if (i < voltage.rows() - 2 && (voltage(i + 1, 1) < v_reset ||
				                               voltage(i + 2, 1) < v_reset)) {
					continue;
				}

				ends.emplace_back(voltage(i - 1, 0));
				started = false;
			}
		}
	}

	// Calculate periods
	for (size_t i = 0; i < ends.size(); i++) {
		diffs.emplace_back(ends[i] - starts[i] - ref_per);
	}

#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> time_voltage;
	for (size_t i = 0; i < voltage.rows(); i++) {
		time_voltage.push_back(
		    std::vector<cypress::Real>{voltage(i, 0), voltage(i, 1)});
	}
	Utilities::write_vector2_to_csv(time_voltage,
	                                _debug_filename("voltage.csv"));

	std::vector<std::vector<cypress::Real>> temp({spike_time});
	Utilities::write_vector2_to_csv(temp, _debug_filename("spikes.csv"));
	Utilities::write_vector_to_csv(diffs, _debug_filename("periods.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_histogram(_debug_filename("periods.csv"), m_backend, false,
	                          -10, "'Lenght of Ref. Per.'");
	Utilities::plot_voltages_spikes(_debug_filename("voltage.csv"), m_backend,
	                                1, 0, _debug_filename("spikes.csv"), 0);
#endif

	if (spike_time.size() == 0) {
		global_logger().warn(
		    "SNABSuite",
		    "Refractory period could not be measured! Adjust parameters.");
		return {std::array<cypress::Real, 4>(
		    {std::numeric_limits<cypress::Real>::quiet_NaN(),
		     std::numeric_limits<cypress::Real>::quiet_NaN()})};
	}

	// Calculate statistics
	cypress::Real max, min, avg, std_dev;
	Utilities::calculate_statistics(diffs, min, max, avg, std_dev);
	return {std::array<cypress::Real, 4>({avg, std_dev, min, max})};
}
}
