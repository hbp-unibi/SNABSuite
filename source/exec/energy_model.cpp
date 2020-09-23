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

#include <cypress/cypress.hpp>

#include "common/benchmark.hpp"
#include "util/read_json.hpp"
#include "util/utilities.hpp"

using namespace SNAB;
using cypress::Json;

std::vector<std::shared_ptr<SNABBase>> snab_vec;

std::shared_ptr<SNABBase> find_snab(const std::string snab_name)
{
	for (auto i : snab_vec) {
		if (i->valid() && snab_name == i->snab_name()) {
			return i;
		}
	}
	throw;  // TODO
}

auto run_snab(const std::string snab_name, const Json &config)
{
	auto snab = find_snab(snab_name)->clone();
	auto config_tar = snab->get_config();
	std::cout << cypress::join(config_tar, config) << std::endl;
	snab->set_config(cypress::join(config_tar, config));
	auto netw = snab->build();
	snab->run();
	return netw;
}

size_t get_number_of_spikes_pop(const cypress::PopulationBase &pop)
{
	size_t spikes = 0;
	if (pop.signals().is_recording(0)) {
		for (const auto &neuron : pop) {
			spikes += neuron.signals().data(0).size();
		}
	}
	return spikes;
}

size_t get_number_of_spikes(cypress::Network &netw, bool sources = true)
{
	size_t spikes = 0;
	for (const auto &pop : netw.populations()) {
		if (&pop.type() == &cypress::SpikeSourceArray::inst() && !sources) {
			continue;
		}
		spikes += get_number_of_spikes_pop(pop);
	}
	return spikes;
}

size_t get_number_of_neurons(cypress::Network &netw, bool sources = true)
{
	size_t neurons = 0;
	for (const auto &pop : netw.populations()) {
		if (&pop.type() == &cypress::SpikeSourceArray::inst() && !sources) {
			continue;
		}
		neurons += pop.size();
	}
	return neurons;
}

std::vector<std::string> required_parameters_vec(
    {"non_spiking", "just_spiking", "input_OneToOne", "input_AllToALL",
     "inter_Single2All", "inter_All2All"});

int main(int argc, const char *argv[])
{
	if (argc < 2 || argc > 3) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR> [bench_index]"
		          << std::endl;
		return 1;
	}
	size_t bench_index = 0;
	if (argc == 3) {
		bench_index = std::stoi(argv[2]);
	}

	std::string simulator = argv[1];
	if (simulator == "spiNNaker") {
		simulator = "spinnaker";
	}
	else if (simulator == "hardware.hbp_pm") {
		simulator = "nmpm1";
	}
	snab_vec = snab_registry(simulator, bench_index);

	// Open config_file
	auto config = read_config("energy_model", simulator);
	bool required_params = check_json_for_parameters(required_parameters_vec,
	                                                 config, "energy_model");
	// Check whether benchmark is labelled as invalid
	if (!((config.find("invalid") == config.end() ||
	       bool(config["invalid"]) == false) &&
	      required_params)) {
		throw;  // TODO
	}

	cypress::global_logger().min_level(cypress::LogSeverity::ERROR, 1);
	double pre_boot, idle, non_spiking_non_rec, non_spiking_rec,
	    full_spiking_non_rec, full_spiking_rec, input_O2O, input_A2A, inter_s2A,
	    inter_A2A;
	std::cout
	    << "Please power cycle the device!\n"
	    << "Now measure the average power consumption over at least 10 sec.\n"
	    << "Please enter the average power-draw: ";
	std::cin >> pre_boot;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "In the following, please measure during the simulation!"
	          << std::endl;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of running idle neurons that are recorded... "
	          << std::endl;
	auto net =
	    run_snab("OutputFrequencyMultipleNeurons", config["non_spiking"]);
	size_t number_of_spikes = get_number_of_spikes(net);
	size_t number_of_neurons = get_number_of_neurons(net);
	if (number_of_spikes > 0) {
		throw std::runtime_error("non_spiking recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> non_spiking_rec;
	non_spiking_rec /= double(number_of_neurons);

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout
	    << "Measuring costs of running idle neurons that are NOT recorded... "
	    << std::endl;
	config["non_spiking"]["record_spikes"] = false;
	net = run_snab("OutputFrequencyMultipleNeurons", config["non_spiking"]);
	number_of_spikes = get_number_of_spikes(net);
	number_of_neurons = get_number_of_neurons(net);
	if (number_of_spikes > 0) {
		throw std::runtime_error("non_spiking recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> non_spiking_non_rec;
	non_spiking_non_rec /= double(number_of_neurons);

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring idle power..." << std::endl;
	std::cout << "Please enter the average power-draw: ";
	std::cin >> idle;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of spikes that are recorded... " << std::endl;
	net = run_snab("OutputFrequencyMultipleNeurons", config["just_spiking"]);
	number_of_spikes = get_number_of_spikes(net);
	number_of_neurons = get_number_of_neurons(net);
	if (number_of_spikes == 0) {
		throw std::runtime_error("just_spiking recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> full_spiking_rec;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of spikes that are NOT recorded... "
	          << std::endl;
	config["just_spiking"]["record_spikes"] = false;
	net = run_snab("OutputFrequencyMultipleNeurons", config["just_spiking"]);
	number_of_spikes = get_number_of_spikes(net);
	number_of_neurons = get_number_of_neurons(net);
	if (number_of_spikes != 0) {
		throw std::runtime_error("just_spiking_not_recording recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> full_spiking_non_rec;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of input spikes one to one..." << std::endl;
	net = run_snab("MaxInputOneToOne", config["input_OneToOne"]);
	number_of_spikes = get_number_of_spikes(net, false);
	if (number_of_spikes != 0) {
		throw std::runtime_error("input_OneToOne recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	number_of_spikes = get_number_of_spikes(net);
	number_of_neurons = get_number_of_neurons(net, false);
	size_t number_of_source_neurons = number_of_neurons;  // OneToOne
	std::cout << "Please enter the average power-draw: ";
	std::cin >> input_O2O;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of input spikes all to all..." << std::endl;
	net = run_snab("MaxInputAllToAll", config["input_AllToALL"]);
	number_of_spikes = get_number_of_spikes(net, false);
	if (number_of_spikes != 0) {
		throw std::runtime_error("input_AllToALL recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	number_of_spikes = get_number_of_spikes(net);
	number_of_neurons = get_number_of_neurons(net, false);
	number_of_source_neurons = number_of_neurons;  // OneToOne
	std::cout << "Please enter the average power-draw: ";
	std::cin >> input_A2A;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of spike transmission one to all..."
	          << std::endl;
	net = run_snab("SingleMaxFreqToGroup", config["inter_Single2All"]);
	number_of_spikes = get_number_of_spikes(net, false);
	if (number_of_spikes == 0) {
		throw std::runtime_error("inter_Single2All recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	size_t number_of_spikes_tar =
	    get_number_of_spikes_pop(net.populations().back());
	if (number_of_spikes_tar != 0) {
		throw std::runtime_error("inter_Single2All recorded " +
		                         std::to_string(number_of_spikes) +
		                         " target spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> inter_s2A;

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	std::cout << "Measuring costs of spike transmission one to one..."
	          << std::endl;
	config["inter_All2All"]["record_spikes"] = true;
	auto net = run_snab("GroupMaxFreqToGroup", config["inter_All2All"]);
	auto number_of_spikes = get_number_of_spikes(net, true);
	if (number_of_spikes == 0) {
		throw std::runtime_error("inter_All2All recorded " +
		                         std::to_string(number_of_spikes) + " spikes");
	}
	auto number_of_spikes_tar =
	    get_number_of_spikes_pop(net.populations().back());
	if (number_of_spikes_tar != 0) {
		throw std::runtime_error("inter_All2All recorded " +
		                         std::to_string(number_of_spikes) +
		                         " target spikes");
	}
	std::cout << "Please enter the average power-draw: ";
	std::cin >> inter_A2A;
	// TODO STDP, Runtime
}
