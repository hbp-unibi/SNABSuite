/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2020  Christoph Ostrau
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

#include <common/snab_registry.hpp>
#include <cypress/backend/power/power.hpp>
#include <cypress/cypress.hpp>
#include <iomanip>

#include "common/snab_base.hpp"
#include "energy/energy_recorder.hpp"
#include "energy/energy_utils.hpp"
#include "energy/fluke_28x.hpp"
#include "util/read_json.hpp"
#include "util/utilities.hpp"

// This activates test mode replacing user inputs (power measurements) and
// network runtimes by fixed values
//#define TESTING

using namespace SNAB;
using cypress::Json;

/**
 * To make measurements on spikey, use a fluke_28x! You need to manipulate 
 * deb-pynn/src/hardware/spikey/pyhal/pyhal_config_s1v2.py:1466
 * Before `myLogger.info('Trigger network emulation.')`:
            import time as time_lib
            import os
            time_lib.sleep(0.5)
            fifo_lock_release = open("sync_lock", 'wb')
            fifo_lock_release.close()
            os.remove("sync_lock")
            time_lib.sleep(0.1)
 * After  myLogger.info('Receive spike data.'):
            with open("sync_lock2", 'w') as file:
                file.write("test")
 * For STDP:
 * spikeyhal/bindings/python/wrappers/pyspikey.cpp:693
 *After flush()
     {
        std::filebuf res;
        res.open("sync_lock", std::ios::out);
        res.close();
        remove("sync_lock");
    }
 * after waitPbFinished()
    std::ofstream("sync_lock2").close();
 */

std::vector<std::shared_ptr<SNABBase>> snab_vec;

/**
 * @brief Find the given SNAB and return a pointer to it
 *
 * @param snab_name The SNAB name to find
 * @return std::shared_ptr< SNAB::SNABBase > ptr to SNAB instance
 */
std::shared_ptr<SNABBase> find_snab(const std::string snab_name)
{
	for (auto i : snab_vec) {
		if (i->valid() && snab_name == i->snab_name()) {
			return i;
		}
	}
	throw std::runtime_error("Internal Error: Snab " + snab_name + "not found");
}

/**
 * @brief Executes a given snab, and replaces values given in the config
 *
 * @param snab_name The SNAB to execute
 * @param config The overwriting config
 * @param setup The overwriting setup
 * @return
 */
auto run_snab(const std::string snab_name, const Json &config,
              const Json &setup)
{
	auto snab = find_snab(snab_name)->clone();
	auto config_tar = snab->get_config();
	snab->set_config(cypress::join(config_tar, config));
	snab->overwrite_backend_config(setup, false);
	auto netw = snab->build();
	snab->run();
	return netw;
}

std::vector<std::string> required_parameters_vec(
    {"non_spiking", "just_spiking", "input_OneToOne", "input_AllToALL",
     "input_random", "inter_Single2All", "inter_One2One", "inter_random",
     "bench_index", "repeat", "setup"});

#ifdef TESTING
double calc_runtime(const cypress::Network &) { return 1.0; }
#else
/**
 * @brief Calculates the runtime of a network in ms. Is now part of cypress
 *
 * @param netw simulated network
 * @return time in ms
 */
double calc_runtime(const cypress::Network &netw)
{

	return netw.runtime().sim_pure * 1.0e3;  // millisecond
}
#endif

/**
 * @brief Runs a small STDP network for measuring STDP related energy costs
 *
 * @param config energy config
 * @param backend simulator backend.
 * @param spike wheter input spikes are active
 * @param setup backend setup configuration
 * @return network instance after the simulation
 */
cypress::Network run_STDP_network(Json config, std::string backend, bool spike,
                                  Json setup)
{
	std::string neuron_type_str = config["neuron_type"];
	cypress::NeuronParameter neuron_params = cypress::NeuronParameter(
	    cypress::SpikingUtils::detect_type(neuron_type_str),
	    config["neuron_params"]);
	cypress::Network netw;
	if (config.find("setup") != config.end()) {
		cypress::join(setup, config["setup"]);
	}

	Utilities::manipulate_backend_string(backend, setup);
	std::vector<cypress::Real> spike_times;
	if (spike) {
		auto n_spikes = config["#spikes"].get<size_t>();
		spike_times = std::vector<cypress::Real>(n_spikes);
		cypress::Real intervall =
		    config["runtime"].get<cypress::Real>() / double(n_spikes);
		for (size_t i = 0; i < n_spikes; i++) {
			spike_times[i] = 1.0 + i * 0.9 * intervall;
		}
	}
	else{
        // TODO only spikey!
        spike_times.push_back(50.0);
        spike_times.push_back(1500000 - 50.0);
    }

	auto pop = cypress::SpikingUtils::add_population(
	    neuron_type_str, netw, neuron_params, config["#neurons"].get<size_t>(),
	    "spikes");
	auto pop_source = netw.create_population<cypress::SpikeSourceArray>(
	    1, cypress::SpikeSourceArrayParameters(spike_times),
	    cypress::SpikeSourceArraySignals().record_spikes());

	auto synapse = cypress::SpikePairRuleAdditive();
    if(config.find("weight")!= config.end()){
        synapse.weight(config["weight"].get<double>());
    }
	netw.add_connection(pop_source, pop,
	                    cypress::Connector::all_to_all(synapse));

	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(backend));
	netw.run(pwbackend, config["runtime"].get<cypress::Real>());
	if (spike) {
        size_t spikes = 0;
		for (const auto &neuron : pop) {
            spikes +=neuron.signals().data(0).size() ;
		}
        if (spikes < 50) {
			throw std::runtime_error(
			    "Wrong configuration of STDP benchmark: found no  spikes!");
		}
	}
	return netw;
}

/**
 * @brief Run a certain SNAB to test the energy model
 *
 * @param config_name name of entry in the energy config
 * @param bench_name SNAB name
 * @param measure_name found in measured in the energy model
 * @param config energy config
 * @param energy_model energy model
 * @param setup simulator setup json
 */
void test_energy_model(const std::string config_name,
                       const std::string bench_name,
                       const std::string measure_name, Json &config,
                       const Json &energy_model, const Json &setup)
{
	config[config_name]["record_spikes"] = true;
	auto net = run_snab(bench_name, config[config_name], setup);
	auto runtime = calc_runtime(net);
#ifdef TESTING
	auto rt = net.runtime();
	rt.sim_pure = runtime * 1e-3;
	net.runtime(rt);
#endif
	auto ener = Energy::calculate_energy(net, energy_model);
	std::cout
	    << "Comparing Values for " << measure_name << ":\nMeasured:\t"
	    << energy_model["measured"][measure_name + "_avg"][0].get<double>() *
	           runtime / 1000.0
	    << "\nCalculated:\t" << std::setprecision(15) << ener.first << " +- "
	    << ener.second << std::endl;
}

/**
 * @brief Check energy model for any invalid numbers. Throws on invalid
 *
 * @param energy_model the model to check
 */
void check_energy_model(const Json &energy_model)
{
	std::vector<std::string> names({"power", "energy"});
	for (const auto &i : names) {
		for (const auto &j : energy_model[i].items()) {
			if (j.value() < 0 || j.value() != j.value()) {
				std::cout << energy_model << std::endl;
				throw std::runtime_error("Negative/NaN value for " + j.key() +
				                         " found! Invalid configuration!");
			}
		}
	}
}

#ifndef TESTING
/**
 * @brief Get a number from std:cin
 *
 * @param  double value, that is returned by the funciton in TESTING mode
 * @return entered value
 */
double number_from_input(double, std::shared_ptr<Energy::Multimeter> &multi,
                         bool threshhold)
{
	double temp;
	if (multi) {
		sleep(2);
		multi->stop_recording();
		double thresh = 0.0;
		if (threshhold) {
			auto min = multi->min_current();
			thresh = min + ((multi->max_current() - min) * 0.7);
			global_logger().info(
			    "EnergyModel",
			    "Measured energy: " +
			        std::to_string(multi->calculate_energy_last(thresh) /
			                       1000.0));
            std::cout << "THRESH "<< thresh <<std::endl;
			return multi->average_power_draw_last(thresh) / 1000.0;
		}
		global_logger().info(
		    "EnergyModel",
		    "Measured energy: " +
		        std::to_string(multi->calculate_energy_last(thresh) / 1000.0));
		return multi->average_power_draw() / 1000.0;
	}
	std::cout << "Please enter the average power-draw in Watt: ";
	std::cin >> temp;
	return temp;
}
#else
double number_from_input(double x, std::shared_ptr<Energy::Multimeter> &,
                         double)
{
	return x;
}
#endif

inline void add(Json &json, double value) { json.emplace_back(value); }

int main(int argc, const char *argv[])
{
	if (argc < 2 || argc > 3) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR> [EnergyConfig]"
		          << std::endl;
		return 1;
	}

	std::string energy_config_path = "";
	if (argc == 3) {
		energy_config_path = argv[2];
	}

	std::string simulator = argv[1];
	if (simulator == "spiNNaker") {
		simulator = "spinnaker";
	}
	else if (simulator == "hardware.hbp_pm") {
		simulator = "nmpm1";
	}
	std::string short_sim = Utilities::split(simulator, '=')[0];

	// Open config_file
	auto config = read_config("energy_model", simulator);
	bool required_params = check_json_for_parameters(required_parameters_vec,
	                                                 config, "energy_model");
	// Check whether benchmark is labelled as invalid
	if (!((config.find("invalid") == config.end() ||
	       bool(config["invalid"]) == false) &&
	      required_params)) {
		throw std::runtime_error(
		    "Invalid config/Marked as invalid or incomplete!");
	}

	cypress::global_logger().min_level(cypress::LogSeverity::INFO, 1);
	Json energy_model = Energy::setup_energy_model();
	size_t bench_index = config["bench_index"].get<size_t>();
	size_t repeat = config["repeat"].get<size_t>();
	Json setup;
	if (config.find("setup") != config.end()) {
		setup = config["setup"];
	}
	if (Utilities::split(simulator, '=').size() > 1) {
		cypress::join(setup, Json::parse(Utilities::split(simulator, '=')[1]));
	}

	snab_vec = snab_registry(simulator, bench_index);
    
    bool threshhold = true;
	std::shared_ptr<Energy::Multimeter> multi;
#ifndef TESTING
	if (config.find("um25c") != config.end()) {
		multi = std::make_shared<Energy::Multimeter>(
		    config["um25c"].get<std::string>());
	}
	if (config.find("fluke_28x") != config.end()) {
		multi = std::make_shared<Energy::Multimeter>(
		    config["fluke_28x"].get<std::string>(),
		    config["fluke_28x_v"].get<double>());
	}
	if (config.find("threshhold") != config.end()) {
		threshhold=
		    config["threshhold"].get<bool>();
	}
#endif

	bool strict_check = true;
	if (config.find("strict_check") != config.end()) {
		strict_check = config["strict_check"].get<bool>();
	}

	if (energy_config_path == "") {
		if (config.find("stdp") != config.end()) {
			energy_model["stdp"] = true;
		}
		Json &measured = energy_model["measured"];
		Json &util = energy_model["util"];

		if (multi) {
			multi->start_recording();
			sleep(20);
			multi->stop_recording();
			measured["pre_boot"] = multi->average_power_draw() / 1000.0;
			global_logger().info(
			    "EnergyModel",
			    "Measured energy: " +
			        std::to_string(multi->calculate_energy_last(0.0) / 1000.0));
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["pre_boot"].back().get<double>() *
			                       20.0));
		}
		else {
			std::cout
			    << "Please power cycle the device!\n"
			    << "Now measure the average power consumption over at least "
			       "10 sec.\n";
			measured["pre_boot"] = number_from_input(1.0, multi, false);
			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout
			    << "In the following, please measure during the simulation!"
			    << std::endl;
		}
		for (size_t iter = 0; iter < repeat; iter++) {
			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
                multi->set_block(true);
				multi->start_recording();
			}
			std::cout << "Measuring costs of running idle neurons that are "
			             " recorded... "
			          << std::endl;
			config["non_spiking"]["record_spikes"] = true;
			auto net = run_snab("OutputFrequencyMultipleNeurons",
			                    config["non_spiking"], setup);
			size_t number_of_spikes = Energy::get_number_of_spikes(net);
			size_t number_of_neurons = Energy::get_number_of_neurons(net);
			if (number_of_spikes > 0) {
				if (strict_check) {
					throw std::runtime_error("non_spiking recorded " +
					                         std::to_string(number_of_spikes) +
					                         " spikes");
				}
				else {
					global_logger().info("EnergyModel",
					                     "non_spiking recorded " +
					                         std::to_string(number_of_spikes) +
					                         " spikes");
				}
			}
			add(measured["non_spiking_rec"],
			    number_from_input(3.0, multi, threshhold));
			add(util["non_spiking_rec"]["number_of_neurons"],
			    number_of_neurons);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["non_spiking_rec"].back().get<double>() *
			            net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring idle power..." << std::endl;
			if (multi) {
				sleep(2);
                multi->set_block(false);
				multi->start_recording();
				sleep(20);
			}
			add(measured["idle"], number_from_input(1.0, multi, false));
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["idle"].back().get<double>() *
			                       20.0));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
                multi->set_block(true);
				multi->start_recording();
			}
			std::cout << "Measuring costs of running idle neurons that are NOT "
			             " recorded... "
			          << std::endl;
			config["non_spiking"]["record_spikes"] = false;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["non_spiking"], setup);
			number_of_spikes = Energy::get_number_of_spikes(net);
			number_of_neurons = Energy::get_number_of_neurons(net);
			if (number_of_spikes > 0) {
				throw std::runtime_error("non_spiking recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			add(measured["non_spiking_non_rec"],
			    number_from_input(2.0, multi, threshhold));
			add(util["non_spiking_non_rec"]["number_of_neurons"],
			    number_of_neurons);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["non_spiking_non_rec"].back().get<double>() *
			            net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of spikes that are recorded... "
			          << std::endl;
			config["just_spiking"]["record_spikes"] = true;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["just_spiking"], setup);
			number_of_spikes = Energy::get_number_of_spikes(net);
			number_of_neurons = Energy::get_number_of_neurons(net);
			if (number_of_spikes == 0) {
				throw std::runtime_error("just_spiking recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			auto runtime = calc_runtime(net);
			add(measured["full_spiking_rec"],
			    number_from_input(5.0, multi, threshhold));
			add(util["full_spiking_rec"]["number_of_neurons"],
			    number_of_neurons);
			add(util["full_spiking_rec"]["runtime"], runtime);
			add(util["full_spiking_rec"]["number_of_spikes"], number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["full_spiking_rec"].back().get<double>() *
			            net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of spikes that are NOT recorded... "
			          << std::endl;
			config["just_spiking"]["record_spikes"] = false;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["just_spiking"], setup);
			auto number_of_spikes2 = Energy::get_number_of_spikes(net);
			// number_of_neurons = get_number_of_neurons(net); //Take from
			// above
			if (number_of_spikes2 != 0) {
				throw std::runtime_error(
				    "just_spiking_not_recording recorded " +
				    std::to_string(number_of_spikes2) + " spikes");
			}
			runtime = calc_runtime(net);
			add(measured["full_spiking_non_rec"],
			    number_from_input(4.0, multi, threshhold));
			add(util["full_spiking_non_rec"]["number_of_neurons"],
			    number_of_neurons);
			add(util["full_spiking_non_rec"]["runtime"], runtime);
			add(util["full_spiking_non_rec"]["number_of_spikes"],
			    number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["full_spiking_non_rec"].back().get<double>() *
			            net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of input spikes one to one... "
			          << std::endl;
			config["input_OneToOne"]["record_spikes"] = true;
			net = run_snab("MaxInputOneToOne", config["input_OneToOne"], setup);
			number_of_spikes2 = Energy::get_number_of_spikes(net, false);
			if (number_of_spikes2 != 0) {
				if (strict_check) {
					throw std::runtime_error("input_OneToOne recorded " +
					                         std::to_string(number_of_spikes2) +
					                         " spikes");
				}
				add(util["input_O2O"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_spikes =
			    Energy::get_number_of_spikes(net) - number_of_spikes2;
			number_of_neurons = Energy::get_number_of_neurons(net, false);
			runtime = calc_runtime(net);
			add(measured["input_O2O"], number_from_input(50.0, multi, threshhold));
			add(util["input_O2O"]["number_of_neurons"], number_of_neurons);
			add(util["input_O2O"]["runtime"], runtime);
			add(util["input_O2O"]["number_of_spikes"], number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["input_O2O"].back().get<double>() *
			                       net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of input spikes all to all... "
			          << std::endl;
			config["input_AllToALL"]["record_spikes"] = true;
			net = run_snab("MaxInputAllToAll", config["input_AllToALL"], setup);
			number_of_spikes = Energy::get_number_of_spikes(net, false);
			if (number_of_spikes != 0) {
				if (strict_check) {
					throw std::runtime_error("input_AllToALL recorded " +
					                         std::to_string(number_of_spikes) +
					                         " spikes");
				}
				add(util["input_A2A"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_spikes = Energy::get_number_of_spikes(net);
			number_of_neurons = Energy::get_number_of_neurons(net, false);
			runtime = calc_runtime(net);
			add(measured["input_A2A"], number_from_input(9.0, multi, threshhold));
			add(util["input_A2A"]["number_of_neurons"], number_of_neurons);
			add(util["input_A2A"]["runtime"], runtime);
			add(util["input_A2A"]["number_of_spikes"], number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["input_A2A"].back().get<double>() *
			                       net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of input spikes random" << std::endl;
			config["input_random"]["record_spikes"] = true;
			net = run_snab("MaxInputFixedOutConnector", config["input_random"],
			               setup);
			number_of_spikes = Energy::get_number_of_spikes(net, false);
			if (number_of_spikes != 0) {
				if (strict_check) {
					throw std::runtime_error("input_random recorded " +
					                         std::to_string(number_of_spikes) +
					                         " spikes");
				}
				add(util["input_random"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_spikes = Energy::get_number_of_spikes(net);
			number_of_neurons = Energy::get_number_of_neurons(net, false);
			runtime = calc_runtime(net);
			add(measured["input_random"], number_from_input(9.0, multi, threshhold));
			add(util["input_random"]["number_of_neurons"], number_of_neurons);
			add(util["input_random"]["runtime"], runtime);
			add(util["input_random"]["number_of_spikes"], number_of_spikes);
			add(util["input_random"]["fan_out"],
			    config["input_random"]["#ConnectionsPerInput"].get<double>());
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["input_random"].back().get<double>() *
			            net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of spike transmission one to all... "
			          << std::endl;
			net = run_snab("SingleMaxFreqToGroup", config["inter_Single2All"],
			               setup);
			number_of_spikes = Energy::get_number_of_spikes(net, false);
			if (number_of_spikes == 0) {
				throw std::runtime_error("inter_Single2All recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			size_t number_of_spikes_tar =
			    Energy::get_number_of_spikes_pop(net.populations().back());
			if (number_of_spikes_tar != 0) {
				if (strict_check) {
					throw std::runtime_error("inter_Single2All recorded " +
					                         std::to_string(number_of_spikes) +
					                         " target spikes");
				}
				add(util["inter_s2A"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_neurons = net.populations().back().size();
			runtime = calc_runtime(net);
			add(measured["inter_s2A"], number_from_input(10.0, multi, threshhold));
			add(util["inter_s2A"]["number_of_neurons"], number_of_neurons);
			add(util["inter_s2A"]["runtime"], runtime);
			add(util["inter_s2A"]["number_of_spikes"], number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["inter_s2A"].back().get<double>() *
			                       net.runtime().sim_pure));

			//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of spike transmission one to one... "
			          << std::endl;
			config["inter_One2One"]["record_spikes"] = true;
			net =
			    run_snab("GroupMaxFreqToGroup", config["inter_One2One"], setup);
			number_of_spikes = Energy::get_number_of_spikes(net, true);
			if (number_of_spikes == 0) {
				throw std::runtime_error("inter_One2One recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			number_of_spikes_tar =
			    Energy::get_number_of_spikes_pop(net.populations().back());
			if (number_of_spikes_tar != 0) {
				if (strict_check) {
					throw std::runtime_error("inter_One2One recorded " +
					                         std::to_string(number_of_spikes) +
					                         " target spikes");
				}
				add(util["inter_O2O"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_neurons = net.populations().back().size();
			runtime = calc_runtime(net);
			add(measured["inter_O2O"], number_from_input(10.0, multi, threshhold));
			add(util["inter_O2O"]["number_of_neurons"], number_of_neurons);
			add(util["inter_O2O"]["runtime"], runtime);
			add(util["inter_O2O"]["number_of_spikes"], number_of_spikes);
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(measured["inter_O2O"].back().get<double>() *
			                       net.runtime().sim_pure));

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (multi) {
				sleep(2);
				multi->start_recording();
			}
			std::cout << "Measuring costs of spike transmission random..."
			          << std::endl;
			config["inter_random"]["record_spikes"] = true;
			net = run_snab("GroupMaxFreqToGroupProb", config["inter_random"],
			               setup);
			number_of_spikes = Energy::get_number_of_spikes(net, true);
			if (number_of_spikes == 0) {
				throw std::runtime_error("inter_random recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			number_of_spikes_tar =
			    Energy::get_number_of_spikes_pop(net.populations().back());
			if (number_of_spikes_tar != 0) {
				if (strict_check) {
					throw std::runtime_error("inter_random recorded " +
					                         std::to_string(number_of_spikes) +
					                         " target spikes");
				}
				add(util["inter_random"]["number_of_spikes_tar"],
				    number_of_spikes2);
			}
			number_of_neurons = Energy::get_number_of_neurons(net, false);
			runtime = calc_runtime(net);
			add(measured["inter_random"], number_from_input(10.0, multi, threshhold));
			add(util["inter_random"]["number_of_neurons"], number_of_neurons);
			add(util["inter_random"]["runtime"], runtime);
			add(util["inter_random"]["number_of_spikes"], number_of_spikes);
			add(util["inter_random"]["connections"],
			    double(net.populations().back().size()) *
			        config["inter_random"]["probability"].get<double>());
			global_logger().info(
			    "EnergyModel",
			    "Calculated energy: " +
			        std::to_string(
			            measured["inter_random"].back().get<double>() *
			            net.runtime().sim_pure));

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (config.find("stdp") != config.end()) {
				if (multi) {
					sleep(2);
                    multi->set_block(true);
					multi->start_recording();
				}
				std::cout << "Measuring costs of idle STDP..." << std::endl;
				auto net =
				    run_STDP_network(config["stdp"], simulator, false, setup);
				auto number_of_neurons =
				    Energy::get_number_of_neurons(net, false);
				add(measured["stdp_idle"], number_from_input(5.0, multi, threshhold));
				add(util["stdp_idle"]["number_of_neurons"], number_of_neurons);

				std::cout << "Measuring costs of running STDP..." << std::endl;
				if (multi) {
					sleep(2);
					multi->start_recording();
				}
				net = run_STDP_network(config["stdp"], simulator, true, setup);
				number_of_neurons = Energy::get_number_of_neurons(net, false);
				auto number_of_spikes =
				    Energy::get_number_of_spikes(net, false);
				auto number_of_source_spikes =
				    Energy::get_number_of_spikes(net, true) - number_of_spikes;
				auto runtime = calc_runtime(net);
				add(measured["stdp_spike"],
				    number_from_input(15.0, multi, threshhold));
				add(util["stdp_spike"]["number_of_neurons"], number_of_neurons);
				add(util["stdp_spike"]["runtime"], runtime);
				add(util["stdp_spike"]["number_of_spikes"], number_of_spikes);
				add(util["stdp_spike"]["number_of_source_spikes"],
				    number_of_source_spikes);
			}
		}
		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		//                         INPUT/OUTPUT
		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		Energy::calculate_coefficients(energy_model);
		check_energy_model(energy_model);
		{
			std::ofstream file;
			file.open(short_sim + "_energy.json", std::ofstream::out);
			if (!file.good()) {
				std::cout << energy_model << std::endl;
				throw std::runtime_error("Could not open " + short_sim +
				                         "_energy.json"
				                         " to write results");
			}
			file << energy_model.dump(4);
			file.close();
		}
	}
	else {
		{
			std::ifstream file;
			file.open(energy_config_path, std::ifstream::in);
			if (!file.good()) {
				throw std::runtime_error("Could not open " +
				                         energy_config_path +
				                         " to read energy model!");
			}
			file >> energy_model;
			file.close();
			check_energy_model(energy_model);
		}
	}
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	//                 TESTING
	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// cypress::global_logger().min_level(cypress::LogSeverity::DEBUG, 1);

	test_energy_model("non_spiking", "OutputFrequencyMultipleNeurons",
	                  "non_spiking_rec", config, energy_model, setup);
	test_energy_model("just_spiking", "OutputFrequencyMultipleNeurons",
	                  "full_spiking_rec", config, energy_model, setup);
	test_energy_model("input_OneToOne", "MaxInputOneToOne", "input_O2O", config,
	                  energy_model, setup);
	test_energy_model("input_AllToALL", "MaxInputAllToAll", "input_A2A", config,
	                  energy_model, setup);
	test_energy_model("input_random", "MaxInputFixedOutConnector",
	                  "input_random", config, energy_model, setup);
	test_energy_model("inter_Single2All", "SingleMaxFreqToGroup", "inter_s2A",
	                  config, energy_model, setup);
	test_energy_model("inter_One2One", "GroupMaxFreqToGroup", "inter_O2O",
	                  config, energy_model, setup);
	test_energy_model("inter_random", "GroupMaxFreqToGroupProb", "inter_random",
	                  config, energy_model, setup);

	auto net = run_STDP_network(config["stdp"], simulator, true, setup);
	auto runtime = calc_runtime(net);
#ifdef TESTING
	auto rt = net.runtime();
	rt.sim_pure = runtime * 1e-3;
	net.runtime(rt);
#endif
	auto ener = Energy::calculate_energy(net, energy_model);
	std::cout << "Comparing Values for STDP:\nMeasured:\t"
	          << energy_model["measured"]["stdp_spike_avg"][0].get<double>() *
	                 runtime / 1000.0
	          << "\nCalculated:\t" << ener.first << " +- " << ener.second
	          << std::endl;

	// auto net = run_snab("MnistSpikey", Json(), setup);
	// std::cout << calculate_energy(net, energy_model)<<std::endl;
}
