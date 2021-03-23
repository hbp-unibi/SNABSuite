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
#include "wta_like.hpp"

#include <cypress/backend/power/power.hpp>  // Control of power via netw
#include <cypress/cypress.hpp>              // Neural network frontend
#include <string>
#include <vector>

#include "util/read_json.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
using cypress::NeuronParameter;
using cypress::SpikingUtils;

using namespace cypress;

SimpleWTA::SimpleWTA(const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend,
               {"Max Winning Streak", "Number of state changes",
                "Time without winner"},
               {"quality", "quality", "quality"},
               {"time", "state changes", "time"}, {"ms", "", "ms"},
               {"neuron_type", "neuron_params", "num_neurons_pop",
                "num_source_neurons", "weight_inp", "weight_self", "weight_inh",
                "prob_inp", "prob_self", "prob_inh", "firing_rate"},
               bench_index)
{
}

cypress::Network &SimpleWTA::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	std::string neuron_type_str = m_config_file["neuron_type"];
	SpikingUtils::detect_type(neuron_type_str);

	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);

	// Get network sizes
	m_num_neurons_pop = m_config_file["num_neurons_pop"];
	m_num_source_neurons = m_config_file["num_source_neurons"];

	// Get firing rate
	m_firing_rate = m_config_file["firing_rate"];

	// Set up population
	m_pop.clear();
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));

	m_pop_source.clear();
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));

	// Read out network data
	m_weight_inp = m_config_file["weight_inp"];
	if (m_config_file.find("delay") != m_config_file.end()) {
		m_delay = m_config_file["delay"];
	}
	m_weight_self = m_config_file["weight_self"];
	m_weight_inh = m_config_file["weight_inh"];
	m_prob_inp = m_config_file["prob_inp"];
	m_prob_self = m_config_file["prob_self"];
	m_prob_inh = m_config_file["prob_inh"];

	// Connecting sources
	netw.add_connection(m_pop_source[0], m_pop[0],
	                    Connector::random(m_weight_inp, m_delay, m_prob_inp));
	netw.add_connection(m_pop_source[1], m_pop[1],
	                    Connector::random(m_weight_inp, m_delay, m_prob_inp));

	// Self connections
	netw.add_connection(m_pop[0], m_pop[0],
	                    Connector::random(m_weight_self, m_delay, m_prob_self));
	netw.add_connection(m_pop[1], m_pop[1],
	                    Connector::random(m_weight_self, m_delay, m_prob_self));

	// Cross connections
	netw.add_connection(m_pop[0], m_pop[1],
	                    Connector::random(m_weight_inh, m_delay, m_prob_inh));
	netw.add_connection(m_pop[1], m_pop[0],
	                    Connector::random(m_weight_inh, m_delay, m_prob_inh));

	return netw;
}

void SimpleWTA::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	try {
		netw.run(pwbackend, m_simulation_length);
	}
	catch (const std::exception &exc) {
		std::cerr << exc.what();
		global_logger().fatal_error(
		    "SNABSuite",
		    "Wrong parameter setting or backend error! Simulation broke down");
	}
}

namespace {
std::vector<size_t> calculate_summed_bins(const PopulationBase &pop,
                                          const Real &simulation_length,
                                          const Real bin_size = 15.0)
{
	std::vector<size_t> bins(size_t((simulation_length - 50.0) / bin_size), 0);
	for (size_t neuron_id = 0; neuron_id < pop.size(); neuron_id++) {
		auto spikes = pop[0][neuron_id].signals().data(0);
		auto temp_bins = SpikingUtils::spike_time_binning<size_t>(
		    50, simulation_length, (simulation_length - 50.0) / bin_size,
		    spikes);
		for (size_t i = 0; i < temp_bins.size(); i++) {
			bins[i] += temp_bins[i];
		}
	}
	return bins;
}
}  // namespace

std::vector<Real> SimpleWTA::calculate_WTA_metrics(
    const std::vector<size_t> &bins, const std::vector<size_t> &bins2,
    const Real bin_size)
{
	bool empty_bins = true;
	for (size_t i = 0; i < bins.size(); i++) {
		if (bins[i] != 0 || bins2[i] != 0) {
			empty_bins = false;
			break;
		}
	}
	if (empty_bins) {
		return std::vector<Real>(3, NaN());
	}

	size_t win_streak_0 = 0, win_streak_1 = 0, max_win_streak = 0;
	size_t num_state_changes = 0, num_time_dead = 0;
	for (size_t i = 0; i < bins.size(); i++) {
		if (bins[i] > 5 + bins2[i]) {
			// Pop 1 is winner
			if (win_streak_0 == 0 && i != 0) {
				// State change
				num_state_changes++;
				if (win_streak_1 > 0) {
					if (win_streak_1 > max_win_streak) {
						max_win_streak = win_streak_1;
					}
					win_streak_1 = 0;
				}
			}
			win_streak_0++;
		}
		else if (bins2[i] > 5 + bins[i]) {
			// Pop2 is winner
			if (win_streak_1 == 0 && i != 0) {
				// State change
				num_state_changes++;
				if (win_streak_0 > 0) {
					if (win_streak_0 > max_win_streak) {
						max_win_streak = win_streak_0;
					}
					win_streak_0 = 0;
				}
			}
			win_streak_1++;
		}
		else {
			// no one is winner
			if (win_streak_0 > 0) {
				num_state_changes++;
				if (win_streak_0 > max_win_streak) {
					max_win_streak = win_streak_0;
				}
			}
			else if (win_streak_1 > 0) {
				num_state_changes++;
				if (win_streak_1 > max_win_streak) {
					max_win_streak = win_streak_1;
				}
			}
			win_streak_0 = 0;
			win_streak_1 = 0;
			num_time_dead++;
		}
	}
	if (win_streak_1 > max_win_streak) {
		max_win_streak = win_streak_1;
	}
	if (win_streak_0 > max_win_streak) {
		max_win_streak = win_streak_0;
	}
	return std::vector<Real>({Real(max_win_streak) * bin_size,
	                          Real(num_state_changes),
	                          Real(num_time_dead) * bin_size});
}

std::vector<std::array<cypress::Real, 4>> SimpleWTA::evaluate()
{
#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop[0].size(); i++) {
		spikes.push_back(m_pop[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop[1].size(); i++) {
		spikes.push_back(m_pop[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));

	spikes.clear();
	;
	for (size_t i = 0; i < m_pop_source[0].size(); i++) {
		spikes.push_back(m_pop_source[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop_source[1].size(); i++) {
		spikes.push_back(m_pop_source[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("source_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("source_spikes.csv"), m_backend);
#endif

	// Time binning of first populations spikes
	auto bins =
	    calculate_summed_bins(m_pop[0], m_simulation_length, m_bin_size);

	// Time binning of second populations spikes
	auto bins2 =
	    calculate_summed_bins(m_pop[1], m_simulation_length, m_bin_size);

#if SNAB_DEBUG
	std::vector<std::vector<size_t>> bins22({bins, bins2});
	Utilities::write_vector2_to_csv(bins22, _debug_filename("bins.csv"));
#endif
	auto res = calculate_WTA_metrics(bins, bins2, m_bin_size);
	return {std::array<cypress::Real, 4>({res[0], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[1], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[2], NaN(), NaN(), NaN()})};
}

LateralInhibWTA::LateralInhibWTA(const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend,
               {"Max Winning Streak", "Number of state changes",
                "Time without winner"},
               {"quality", "quality", "quality"},
               {"time", "state changes", "time"}, {"ms", "", "ms"},
               {"neuron_type", "neuron_params", "num_neurons_pop",
                "num_source_neurons", "weight_inp", "weight_self",
                "weight_lat_inh", "weight_lat_exc", "prob_inp", "prob_self",
                "prob_lat_exc", "firing_rate", "num_inhibitory_neurons"},
               bench_index),
      m_inhibit_pop(m_netw, 0)
{
}

Network &LateralInhibWTA::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	std::string neuron_type_str = m_config_file["neuron_type"];
	SpikingUtils::detect_type(neuron_type_str);

	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);

	// Get network sizes
	m_num_neurons_pop = m_config_file["num_neurons_pop"];
	m_num_source_neurons = m_config_file["num_source_neurons"];
	m_num_inhibitory_neurons = m_config_file["num_inhibitory_neurons"];

	// Get firing rate
	m_firing_rate = m_config_file["firing_rate"];

	// Set up population
	m_pop.clear();
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));

	m_pop_source.clear();
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));

	m_inhibit_pop =
	    SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                 m_num_inhibitory_neurons, "spikes");

	// Read out network data
	m_weight_inp = m_config_file["weight_inp"];
	if (m_config_file.find("delay") != m_config_file.end()) {
		m_delay = m_config_file["delay"];
	}
	m_weight_self = m_config_file["weight_self"];
	m_weight_lat_inh = m_config_file["weight_lat_inh"];
	m_weight_lat_exc = m_config_file["weight_lat_exc"];
	m_prob_inp = m_config_file["prob_inp"];
	m_prob_self = m_config_file["prob_self"];
	m_prob_lat_exc = m_config_file["prob_lat_exc"];

	// Connecting sources
	netw.add_connection(m_pop_source[0], m_pop[0],
	                    Connector::random(m_weight_inp, m_delay, m_prob_inp));
	netw.add_connection(m_pop_source[1], m_pop[1],
	                    Connector::random(m_weight_inp, m_delay, m_prob_inp));

	// Self connections
	netw.add_connection(m_pop[0], m_pop[0],
	                    Connector::random(m_weight_self, m_delay, m_prob_self));
	netw.add_connection(m_pop[1], m_pop[1],
	                    Connector::random(m_weight_self, m_delay, m_prob_self));

	// Excite inhibitory Population
	netw.add_connection(
	    m_pop[0], m_inhibit_pop,
	    Connector::random(m_weight_lat_exc, m_delay, m_prob_lat_exc));
	netw.add_connection(
	    m_pop[1], m_inhibit_pop,
	    Connector::random(m_weight_lat_exc, m_delay, m_prob_lat_exc));

	// Inhibit populations
	netw.add_connection(m_inhibit_pop, m_pop[0],
	                    Connector::all_to_all(m_weight_lat_inh, m_delay));
	netw.add_connection(m_inhibit_pop, m_pop[1],
	                    Connector::all_to_all(m_weight_lat_inh, m_delay));

	return netw;
}

void LateralInhibWTA::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	try {
		netw.run(pwbackend, m_simulation_length);
	}
	catch (const std::exception &exc) {
		std::cerr << exc.what();
		global_logger().fatal_error(
		    "SNABSuite",
		    "Wrong parameter setting or backend error! Simulation broke down");
	}
}
std::vector<std::array<cypress::Real, 4>> LateralInhibWTA::evaluate()
{
#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop[0].size(); i++) {
		spikes.push_back(m_pop[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop[1].size(); i++) {
		spikes.push_back(m_pop[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));

	spikes.clear();
	;
	for (size_t i = 0; i < m_pop_source[0].size(); i++) {
		spikes.push_back(m_pop_source[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop_source[1].size(); i++) {
		spikes.push_back(m_pop_source[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("source_spikes.csv"));

	spikes.clear();
	for (size_t i = 0; i < m_inhibit_pop.size(); i++) {
		spikes.push_back(m_inhibit_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("inhibi_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("source_spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("inhibi_spikes.csv"), m_backend);
#endif

	// Time binning of first populations spikes
	auto bins =
	    calculate_summed_bins(m_pop[0], m_simulation_length, m_bin_size);

	// Time binning of second populations spikes
	auto bins2 =
	    calculate_summed_bins(m_pop[1], m_simulation_length, m_bin_size);

#if SNAB_DEBUG
	std::vector<std::vector<size_t>> bins22({bins, bins2});
	Utilities::write_vector2_to_csv(bins22, _debug_filename("bins.csv"));
#endif
	auto res = SimpleWTA::calculate_WTA_metrics(bins, bins2, m_bin_size);
	return {std::array<cypress::Real, 4>({res[0], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[1], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[2], NaN(), NaN(), NaN()})};
}

MirrorInhibWTA::MirrorInhibWTA(const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend,
               {"Max Winning Streak", "Number of state changes",
                "Time without winner"},
               {"quality", "quality", "quality"},
               {"time", "state changes", "time"}, {"ms", "", "ms"},
               {"neuron_type", "neuron_params", "num_neurons_pop",
                "num_source_neurons", "weight_inp", "weight_self",
                "weight_to_inh", "weight_from_inh", "prob_inp", "prob_self",
                "prob_to_inh", "firing_rate", "num_inhibitory_neurons"},
               bench_index)
{
}

MirrorInhibWTA::MirrorInhibWTA(const std::string backend, size_t bench_index,
                               std::string snab_name)
    : SNABBase(snab_name, backend,
               {"Max Winning Streak", "Number of state changes",
                "Time without winner"},
               {"quality", "quality", "quality"},
               {"time", "state changes", "time"}, {"ms", "", "ms"},
               {"neuron_type", "neuron_params", "num_neurons_pop",
                "num_source_neurons", "weight_inp", "weight_self",
                "weight_to_inh", "weight_from_inh", "prob_inp", "prob_self",
                "prob_to_inh", "firing_rate", "num_inhibitory_neurons"},
               bench_index)
{
}

Network &MirrorInhibWTA::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	std::string neuron_type_str = m_config_file["neuron_type"];
	SpikingUtils::detect_type(neuron_type_str);

	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);

	// Get network sizes
	m_num_neurons_pop = m_config_file["num_neurons_pop"];
	m_num_source_neurons = m_config_file["num_source_neurons"];
	m_num_inhibitory_neurons = m_config_file["num_inhibitory_neurons"];

	// Get firing rate
	m_firing_rate = m_config_file["firing_rate"];

	// Set up population
	m_pop.clear();
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));
	m_pop.push_back(SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, m_num_neurons_pop, "spikes"));

	m_pop_source.clear();
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));
	m_pop_source.push_back(netw.create_population<SpikeSourcePoisson>(
	    m_num_source_neurons,
	    SpikeSourcePoissonParameters()
	        .rate(m_firing_rate)
	        .start(10)
	        .duration(m_simulation_length - 11.0),
	    SpikeSourcePoissonSignals({"spikes"})));

	m_inhibit_pop.push_back(
	    SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                 m_num_inhibitory_neurons, "spikes"));
	m_inhibit_pop.push_back(
	    SpikingUtils::add_population(neuron_type_str, netw, m_neuro_params,
	                                 m_num_inhibitory_neurons, "spikes"));

	// Read out network data
	m_weight_inp = m_config_file["weight_inp"];
	if (m_config_file.find("delay") != m_config_file.end()) {
		m_delay = m_config_file["delay"];
	}
	m_weight_self = m_config_file["weight_self"];
	m_weight_to_inh = m_config_file["weight_to_inh"];
	m_weight_from_inh = m_config_file["weight_from_inh"];
	m_prob_inp = m_config_file["prob_inp"];
	m_prob_self = m_config_file["prob_self"];
	m_prob_to_inh = m_config_file["prob_to_inh"];

	// Connecting sources
	if (m_prob_inp) {
		netw.add_connection(
		    m_pop_source[0], m_pop[0],
		    Connector::random(m_weight_inp, m_delay, m_prob_inp));
		netw.add_connection(
		    m_pop_source[1], m_pop[1],
		    Connector::random(m_weight_inp, m_delay, m_prob_inp));
	}
	else {
		netw.add_connection(m_pop_source[0], m_pop[0],
		                    Connector::one_to_one(m_weight_inp, m_delay));
		netw.add_connection(m_pop_source[1], m_pop[1],
		                    Connector::one_to_one(m_weight_inp, m_delay));
	}

	// Self connections
	if (m_prob_self) {
		netw.add_connection(
		    m_pop[0], m_pop[0],
		    Connector::random(m_weight_self, m_delay, m_prob_self));
		netw.add_connection(
		    m_pop[1], m_pop[1],
		    Connector::random(m_weight_self, m_delay, m_prob_self));
	}
	else {
		netw.add_connection(m_pop[0], m_pop[0],
		                    Connector::one_to_one(m_weight_self, m_delay));
		netw.add_connection(m_pop[1], m_pop[1],
		                    Connector::one_to_one(m_weight_self, m_delay));
	}

	// Excite inhibitory populations
	netw.add_connection(
	    m_pop[0], m_inhibit_pop[0],
	    Connector::random(m_weight_to_inh, m_delay, m_prob_to_inh));
	netw.add_connection(
	    m_pop[1], m_inhibit_pop[1],
	    Connector::random(m_weight_to_inh, m_delay, m_prob_to_inh));

	// Inhibit populations
	netw.add_connection(m_inhibit_pop[1], m_pop[0],
	                    Connector::all_to_all(m_weight_from_inh, m_delay));
	netw.add_connection(m_inhibit_pop[0], m_pop[1],
	                    Connector::all_to_all(m_weight_from_inh, m_delay));

	return netw;
}

void MirrorInhibWTA::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	try {
		netw.run(pwbackend, m_simulation_length);
	}
	catch (const std::exception &exc) {
		std::cerr << exc.what();
		global_logger().fatal_error(
		    "SNABSuite",
		    "Wrong parameter setting or backend error! Simulation broke down");
	}
}
std::vector<std::array<cypress::Real, 4>> MirrorInhibWTA::evaluate()
{
#if SNAB_DEBUG
	// Write data to files
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < m_pop[0].size(); i++) {
		spikes.push_back(m_pop[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop[1].size(); i++) {
		spikes.push_back(m_pop[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));

	spikes.clear();
	;
	for (size_t i = 0; i < m_pop_source[0].size(); i++) {
		spikes.push_back(m_pop_source[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_pop_source[1].size(); i++) {
		spikes.push_back(m_pop_source[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("source_spikes.csv"));

	spikes.clear();
	for (size_t i = 0; i < m_inhibit_pop[0].size(); i++) {
		spikes.push_back(m_inhibit_pop[0][i].signals().data(0));
	}
	for (size_t i = 0; i < m_inhibit_pop[1].size(); i++) {
		spikes.push_back(m_inhibit_pop[1][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes,
	                                _debug_filename("inhibi_spikes.csv"));

	// Trigger plots
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("source_spikes.csv"), m_backend);
	Utilities::plot_spikes(_debug_filename("inhibi_spikes.csv"), m_backend);
#endif

	// Time binning of first populations spikes
	auto bins =
	    calculate_summed_bins(m_pop[0], m_simulation_length, m_bin_size);

	// Time binning of second populations spikes
	auto bins2 =
	    calculate_summed_bins(m_pop[1], m_simulation_length, m_bin_size);

#if SNAB_DEBUG
	std::vector<std::vector<size_t>> bins22({bins, bins2});
	Utilities::write_vector2_to_csv(bins22, _debug_filename("bins.csv"));
#endif
	auto res = SimpleWTA::calculate_WTA_metrics(bins, bins2, m_bin_size);
	return {std::array<cypress::Real, 4>({res[0], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[1], NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({res[2], NaN(), NaN(), NaN()})};
}

}  // namespace SNAB
