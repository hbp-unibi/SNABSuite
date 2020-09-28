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

#include <cypress/backend/power/power.hpp>
#include <cypress/cypress.hpp>

//#include "common/benchmark.hpp"
#include <common/snab_registry.hpp>
#include <iomanip>

#include "common/snab_base.hpp"
#include "util/read_json.hpp"
#include "util/utilities.hpp"

#define TESTING  // TODO

// TODO Accurate timing of NEST simulation

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
	throw std::runtime_error("Internal Error: Snab " + snab_name + "not found");
}

auto run_snab(const std::string snab_name, const Json &config)
{
	auto snab = find_snab(snab_name)->clone();
	auto config_tar = snab->get_config();
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
     "inter_Single2All", "inter_One2One", "time_scale_factor", "bench_index",
     "repeat"});

#ifdef TESTING
double calc_runtime(const std::string, const double, const double,
                    const cypress::Network &)
{
	return 1.0;
}
#else
double calc_runtime(const std::string simulator, const double runtime,
                    const double time_scale_factor,
                    const cypress::Network &netw)
{
	if (simulator == "genn" || simulator == "pynn.nest" ||
	    simulator == "nest") {
		return netw.runtime().sim * 1000.0;  // millisecond
	}
	else {
		return runtime * time_scale_factor;
	}
}
#endif

cypress::Network run_STDP_network(Json config, std::string backend, bool spike)
{
	std::string neuron_type_str = config["neuron_type"];
	cypress::NeuronParameter neuron_params = cypress::NeuronParameter(
	    cypress::SpikingUtils::detect_type(neuron_type_str),
	    config["neuron_params"]);
	cypress::Network netw;
	std::vector<cypress::Real> spike_times;
	if (spike) {
		spike_times = config["spike_times"].get<std::vector<cypress::Real>>();
	}
	auto pop = cypress::SpikingUtils::add_population(
	    neuron_type_str, netw, neuron_params, config["#neurons"].get<size_t>(),
	    "spikes");
	auto pop_source = netw.create_population<cypress::SpikeSourceArray>(
	    config["#neurons"].get<size_t>(),
	    cypress::SpikeSourceArrayParameters(spike_times),
	    cypress::SpikeSourceArraySignals().record_spikes());

	auto synapse = cypress::SpikePairRuleAdditive();
	netw.add_connection(pop_source, pop,
	                    cypress::Connector::one_to_one(synapse));

	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(backend));
	netw.run(pwbackend, config["runtime"].get<cypress::Real>());
	if (spike) {
		for (const auto &neuron : pop) {
			if (neuron.signals().data(0).size() == 0) {
				throw std::runtime_error(
				    "Wrong configuration of STDP benchmark: found no  spikes "
				    "for neuron " +
				    std::to_string(neuron.nid()) + "!");
			}
		}
	}
	return netw;
}
double calculate_energy(const cypress::Network &netw, const Json &energy_model);

void test_energy_model(const std::string config_name,
                       const std::string bench_name,
                       const std::string measure_name, Json &config,
                       const Json &energy_model, std::string short_sim,
                       double time_scale_factor)
{
	config[config_name]["record_spikes"] = true;
	auto net = run_snab(bench_name, config[config_name]);
	auto ener = calculate_energy(net, energy_model);
	auto runtime =
	    calc_runtime(short_sim, config[config_name]["runtime"].get<double>(),
	                 time_scale_factor, net);
	std::cout << "Comparing Values for " << measure_name << ":\nMeasured:\t"
	          << energy_model["measured"][measure_name].get<double>() *
	                 runtime / 1000.0
	          << "\nCalculated:\t" << std::setprecision(15) << ener
	          << std::endl;
}

void check_energy_model(const Json &energy_model)
{
	std::vector<std::string> names({"measured", "power", "energy"});
	for (const auto &i : names) {
		for (const auto &j : energy_model[i].items()) {
			if (j.value() < 0) {
				std::cout << energy_model << std::endl;
				throw std::runtime_error("Negative value for " + j.key() +
				                         " found! Invalid configuration!");
			}
		}
	}
}


void normalize(Json &json, double norm)
{
	for (auto &i : json.items()) {
		i.value() = i.value().get<double>() / norm;
	}
}
void normalize2(Json &json, double norm)
{
	for (auto &i : json.items()) {
		for (auto &j : i.value().items()) {
			j.value() = j.value().get<double>() / norm;
		}
	}
}

#ifndef TESTING
double number_from_input(double)
{
	double temp;
	std::cout << "Please enter the average power-draw in Watt: ";
	std::cin >> temp;
	return temp;
}
#else
double number_from_input(double x) { return x; }
#endif

inline void add(Json &json, double value) { json = json.get<double>() + value; }
Json setup_energy_model();
void calculate_coefficients(Json &energy_model, size_t repeat);

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

	cypress::global_logger().min_level(cypress::LogSeverity::ERROR, 1);
	Json energy_model = setup_energy_model();
	size_t bench_index = config["bench_index"].get<size_t>();
	double time_scale_factor = config["time_scale_factor"].get<double>();
	size_t repeat = config["repeat"].get<size_t>();
	snab_vec = snab_registry(simulator, bench_index);

	if (energy_config_path == "") {
		if (config.find("stdp") != config.end()) {
			energy_model["stdp"] = true;
		}
		Json &measured = energy_model["measured"];
		Json &util = energy_model["util"];

		std::cout << "Please power cycle the device!\n"
		          << "Now measure the average power consumption over at least "
		             "10 sec.\n";
		measured["pre_boot"] = number_from_input(1.0);

		// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		std::cout << "In the following, please measure during the simulation!"
		          << std::endl;

		for (size_t iter = 0; iter < repeat; iter++) {

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of running idle neurons that are "
			             "recorded... "
			          << std::endl;
			config["non_spiking"]["record_spikes"] = true;
			auto net = run_snab("OutputFrequencyMultipleNeurons",
			                    config["non_spiking"]);
			size_t number_of_spikes = get_number_of_spikes(net);
			size_t number_of_neurons = get_number_of_neurons(net);
			if (number_of_spikes > 0) {
				throw std::runtime_error("non_spiking recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			add(measured["non_spiking_rec"], number_from_input(3.0));
			add(util["non_spiking_rec"]["number_of_neurons"],
			    number_of_neurons);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring idle power..." << std::endl;
			add(measured["idle"], number_from_input(1.0));

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of running idle neurons that are NOT "
			             "recorded... "
			          << std::endl;
			config["non_spiking"]["record_spikes"] = false;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["non_spiking"]);
			number_of_spikes = get_number_of_spikes(net);
			number_of_neurons = get_number_of_neurons(net);
			if (number_of_spikes > 0) {
				throw std::runtime_error("non_spiking recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			add(measured["non_spiking_non_rec"], number_from_input(2.0));
			add(util["non_spiking_non_rec"]["number_of_neurons"],
			    number_of_neurons);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of spikes that are recorded... "
			          << std::endl;
			config["just_spiking"]["record_spikes"] = true;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["just_spiking"]);
			number_of_spikes = get_number_of_spikes(net);
			number_of_neurons = get_number_of_neurons(net);
			if (number_of_spikes == 0) {
				throw std::runtime_error("just_spiking recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			auto runtime = calc_runtime(
			    short_sim, config["just_spiking"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["full_spiking_rec"], number_from_input(5.0));
			add(util["full_spiking_rec"]["number_of_neurons"],
			    number_of_neurons);
			add(util["full_spiking_rec"]["runtime"], runtime);
			add(util["full_spiking_rec"]["number_of_spikes"], number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of spikes that are NOT recorded... "
			          << std::endl;
			config["just_spiking"]["record_spikes"] = false;
			net = run_snab("OutputFrequencyMultipleNeurons",
			               config["just_spiking"]);
			auto number_of_spikes2 = get_number_of_spikes(net);
			// number_of_neurons = get_number_of_neurons(net); // Take from
			// above
			if (number_of_spikes2 != 0) {
				throw std::runtime_error(
				    "just_spiking_not_recording recorded " +
				    std::to_string(number_of_spikes2) + " spikes");
			}
			runtime = calc_runtime(
			    short_sim, config["just_spiking"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["full_spiking_non_rec"], number_from_input(4.0));
			add(util["full_spiking_non_rec"]["number_of_neurons"],
			    number_of_neurons);
			add(util["full_spiking_non_rec"]["runtime"], runtime);
			add(util["full_spiking_non_rec"]["number_of_spikes"],
			    number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of input spikes one to one..."
			          << std::endl;
			config["input_OneToOne"]["record_spikes"] = true;
			net = run_snab("MaxInputOneToOne", config["input_OneToOne"]);
			number_of_spikes = get_number_of_spikes(net, false);
			if (number_of_spikes != 0) {
				throw std::runtime_error("input_OneToOne recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			number_of_spikes = get_number_of_spikes(net);
			number_of_neurons = get_number_of_neurons(net, false);
			runtime = calc_runtime(
			    short_sim, config["input_OneToOne"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["input_O2O"], number_from_input(7.0));
			add(util["input_O2O"]["number_of_neurons"], number_of_neurons);
			add(util["input_O2O"]["runtime"], runtime);
			add(util["input_O2O"]["number_of_spikes"], number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of input spikes all to all..."
			          << std::endl;
			config["input_AllToALL"]["record_spikes"] = true;
			net = run_snab("MaxInputAllToAll", config["input_AllToALL"]);
			number_of_spikes = get_number_of_spikes(net, false);
			if (number_of_spikes != 0) {
				throw std::runtime_error("input_AllToALL recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			number_of_spikes = get_number_of_spikes(net);
			number_of_neurons = get_number_of_neurons(net, false);
			runtime = calc_runtime(
			    short_sim, config["input_AllToALL"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["input_A2A"], number_from_input(9.0));
			add(util["input_A2A"]["number_of_neurons"], number_of_neurons);
			add(util["input_A2A"]["runtime"], runtime);
			add(util["input_A2A"]["number_of_spikes"], number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of spike transmission one to all..."
			          << std::endl;
			net = run_snab("SingleMaxFreqToGroup", config["inter_Single2All"]);
			number_of_spikes = get_number_of_spikes(net, false);
			if (number_of_spikes == 0) {
				throw std::runtime_error("inter_Single2All recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			size_t number_of_spikes_tar =
			    get_number_of_spikes_pop(net.populations().back());
			if (number_of_spikes_tar != 0) {
				throw std::runtime_error("inter_Single2All recorded " +
				                         std::to_string(number_of_spikes) +
				                         " target spikes");
			}
			number_of_neurons = net.populations().back().size();
			runtime = calc_runtime(
			    short_sim, config["inter_Single2All"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["inter_s2A"], number_from_input(10.0));
			add(util["inter_s2A"]["number_of_neurons"], number_of_neurons);
			add(util["inter_s2A"]["runtime"], runtime);
			add(util["inter_s2A"]["number_of_spikes"], number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			std::cout << "Measuring costs of spike transmission one to one..."
			          << std::endl;
			config["inter_One2One"]["record_spikes"] = true;
			net = run_snab("GroupMaxFreqToGroup", config["inter_One2One"]);
			number_of_spikes = get_number_of_spikes(net, true);
			if (number_of_spikes == 0) {
				throw std::runtime_error("inter_One2One recorded " +
				                         std::to_string(number_of_spikes) +
				                         " spikes");
			}
			number_of_spikes_tar =
			    get_number_of_spikes_pop(net.populations().back());
			if (number_of_spikes_tar != 0) {
				throw std::runtime_error("inter_One2One recorded " +
				                         std::to_string(number_of_spikes) +
				                         " target spikes");
			}
			number_of_neurons = net.populations().back().size();
			runtime = calc_runtime(
			    short_sim, config["inter_One2One"]["runtime"].get<double>(),
			    time_scale_factor, net);
			add(measured["inter_O2O"], number_from_input(10.0));
			add(util["inter_O2O"]["number_of_neurons"], number_of_neurons);
			add(util["inter_O2O"]["runtime"], runtime);
			add(util["inter_O2O"]["number_of_spikes"], number_of_spikes);

			// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
			if (config.find("stdp") != config.end()) {
				std::cout << "Measuring costs of idle STDP..." << std::endl;
				net = run_STDP_network(config["stdp"], simulator, false);
				number_of_neurons = get_number_of_neurons(net, false);
				add(measured["stdp_idle"], number_from_input(5.0));
				add(util["stdp_idle"]["number_of_neurons"], number_of_neurons);

				std::cout << "Measuring costs of running STDP..." << std::endl;
				net = run_STDP_network(config["stdp"], simulator, true);
				number_of_neurons = get_number_of_neurons(net, false);
				number_of_spikes = get_number_of_spikes(net, false);
				auto number_of_source_spikes =
				    get_number_of_spikes(net, true) - number_of_spikes;
				runtime = calc_runtime(short_sim,
				                       config["stdp"]["runtime"].get<double>(),
				                       time_scale_factor, net);
				add(measured["stdp_spike"], number_from_input(15.0));
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
		calculate_coefficients(energy_model, repeat);
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
	                  "non_spiking_rec", config, energy_model, short_sim,
	                  time_scale_factor);
	test_energy_model("just_spiking", "OutputFrequencyMultipleNeurons",
	                  "full_spiking_rec", config, energy_model, short_sim,
	                  time_scale_factor);
	test_energy_model("input_OneToOne", "MaxInputOneToOne", "input_O2O", config,
	                  energy_model, short_sim, time_scale_factor);
	test_energy_model("input_AllToALL", "MaxInputAllToAll", "input_A2A", config,
	                  energy_model, short_sim, time_scale_factor);
	test_energy_model("inter_Single2All", "SingleMaxFreqToGroup", "inter_s2A",
	                  config, energy_model, short_sim, time_scale_factor);
	test_energy_model("inter_One2One", "GroupMaxFreqToGroup", "inter_O2O",
	                  config, energy_model, short_sim, time_scale_factor);

	auto net = run_STDP_network(config["stdp"], simulator, true);
	auto ener = calculate_energy(net, energy_model);
	auto runtime =
	    calc_runtime(short_sim, config["stdp"]["runtime"].get<double>(),
	                 time_scale_factor, net);
	std::cout << "Comparing Values for STDP:\nMeasured:\t"
	          << energy_model["measured"]["stdp_spike"].get<double>() *
	                 runtime / 1000.0
	          << "\nCalculated:\t" << ener << std::endl;
}

std::vector<size_t> conn_ids_source(
    const size_t source_id,
    const std::vector<cypress::ConnectionDescriptor> &conns)
{
	std::vector<size_t> res;
	for (size_t i = 0; i < conns.size(); i++) {
		if (size_t(conns[i].pid_src()) == source_id) {
			res.push_back(i);
		}
	}
	return res;
}

std::pair<size_t, size_t> calc_postsyn_spikes(
    const cypress::PopulationBase &pop,
    const std::vector<cypress::ConnectionDescriptor> &conns, bool stdp)
{
	auto conn_ids = conn_ids_source(pop.pid(), conns);
	size_t spikes = 0, spikes_all = 0;
	const auto &netw = pop.network();
	for (auto cid : conn_ids) {
		auto name = conns[cid].connector().name();
		/*if (conns[cid].connector().synapse()->learning() && !stdp) {
		    continue;
		}*/
		if (stdp && !(conns[cid].connector().synapse()->learning())) {
			continue;
		}
		if (name == "AllToAllConnector") {
			size_t tar_size = netw.populations()[conns[cid].pid_tar()].size();
			for (const auto &neuron : pop) {
				spikes_all += neuron.signals().data(0).size() * tar_size;
			}
		}
		else if (name == "OneToOneConnector") {
			for (const auto &neuron : pop) {
				spikes += neuron.signals().data(0).size();
			}
		}
		else if (name == "FixedFanOutConnector") {
			size_t fan_out =
			    size_t(conns[cid].connector().additional_parameter());
			for (const auto &neuron : pop) {
				spikes += neuron.signals().data(0).size() * fan_out;
			}
		}
		else {
			if (name != "FromListConnector") {
				cypress::global_logger().warn(
				    "EnergyModel",
				    "Energy for random connectors is only approximated!");
			}
			std::vector<cypress::LocalConnection> connections;
			conns[cid].connect(connections);
			for (auto lc : connections) {
				spikes += pop[lc.src].signals().data(0).size();
			}
		}
	}
	return std::pair<size_t, size_t>(spikes, spikes_all);
}

size_t calc_number_stdp_synapses(
    const cypress::Network &netw,
    const std::vector<cypress::ConnectionDescriptor> &conns)
{

	size_t res = 0;
	for (const auto &descr : conns) {
		auto name = descr.connector().name();
		if (descr.connector().synapse()->learning()) {
			if (name == "AllToAllConnector") {
				res += netw.populations()[descr.pid_tar()].size() *
				       netw.populations()[descr.pid_src()].size();
			}
			else if (name == "OneToOneConnector") {
				res += netw.populations()[descr.pid_tar()].size();
			}
			else {
				if (name != "FromListConnector") {
					cypress::global_logger().warn(
					    "EnergyModel",
					    "Energy for random connectors is only approximated!");
				}
				std::vector<cypress::LocalConnection> connections;
				descr.connect(connections);
				res += connections.size();
			}
		}
	}
	return res;
}

double calculate_energy(const cypress::Network &netw, const Json &energy_model)
{
	double runtime =
	    netw.runtime().sim * 1000.0;  // TODO for NEST, BS, Spikey...
#ifdef TESTING
	runtime = 1.0;
#endif
	double energy = 0.0;
	energy += runtime * energy_model["power"]["idle"].get<double>();
	auto &conns = netw.connections();
	for (const auto &pop : netw.populations()) {
		if (&pop.type() == &cypress::SpikeSourceArray::inst()) {
			if (!pop.signals().is_recording(0)) {
				cypress::global_logger().warn(
				    "EnergyModel",
				    "Please activate spike recording for all populations!");
				continue;
			}
			auto spikes = calc_postsyn_spikes(pop, conns, false);
			energy += double(std::get<0>(spikes)) *
			          energy_model["energy"]["InputSpike_O2O"].get<double>();
			energy += double(std::get<1>(spikes)) *
			          energy_model["energy"]["InputSpike_A2A"].get<double>();
		}
		else {
			if (!pop.signals().is_recording(0)) {
				energy += double(pop.size()) *
				          energy_model["power"]["idle_neurons"].get<double>() *
				          runtime;
				cypress::global_logger().warn(
				    "EnergyModel",
				    "Please activate spike recording for all populations!");
				continue;
			}
			energy +=
			    double(pop.size()) *
			    energy_model["power"]["idle_recorded_neurons"].get<double>() *
			    runtime;

			auto spikes = get_number_of_spikes_pop(pop);
			energy +=
			    double(spikes) * energy_model["energy"]["spike"].get<double>();
			auto spikes2 = calc_postsyn_spikes(pop, conns, false);
			energy += double(std::get<0>(spikes2)) *
			          energy_model["energy"]["Transmission_O2O"].get<double>();
			energy += double(std::get<1>(spikes2)) *
			          energy_model["energy"]["Transmission_S2A"].get<double>();
		}
	}

	if (energy_model["stdp"].get<bool>()) {
		energy += double(calc_number_stdp_synapses(netw, conns)) *
		          energy_model["power"]["idle_stdp"].get<double>();
		for (const auto &pop : netw.populations()) {
			if (pop.signals().is_recording(0)) {
				auto spikes2 = calc_postsyn_spikes(pop, conns, true);
				energy +=
				    double(std::get<0>(spikes2)) *
				    energy_model["energy"]["Transmission_STDP"].get<double>();
				energy +=
				    double(std::get<1>(spikes2)) *
				    energy_model["energy"]["Transmission_STDP"].get<double>();
			}
		}
	}
	return energy;
}


Json setup_energy_model()
{
	Json energy_model;
	energy_model["stdp"] = false;
	energy_model["measured"] = {};
	energy_model["power"] = {};
	energy_model["energy"] = {};
	Json &measured = energy_model["measured"];
	Json &power = energy_model["power"];
	Json &energy = energy_model["energy"];
	measured["pre_boot"] = 0.0;
	power["pre_boot"] = 0.0;
	measured["non_spiking_rec"] = 0.0;
	measured["idle"] = 0.0;
	power["idle"] = 0.0;
	power["idle_recorded_neurons"] = 0.0;
	measured["non_spiking_non_rec"] = 0.0;
	power["idle_neurons"] = 0.0;
	measured["full_spiking_rec"] = 0.0;
	energy["spike"] = 0.0;
	measured["full_spiking_non_rec"] = 0.0;
	energy["not_recorded_spike"] = 0.0;
	measured["input_O2O"] = 0.0;
	energy["InputSpike_O2O"] = 0.0;
	measured["input_A2A"] = 0.0;
	energy["InputSpike_A2A"] = 0.0;
	measured["inter_s2A"] = 0.0;
	energy["Transmission_S2A"] = 0.0;
	measured["inter_O2O"] = 0.0;
	energy["Transmission_O2O"] = 0.0;
	measured["stdp_idle"] = 0.0;
	power["idle_stdp"] = 0.0;
	measured["stdp_spike"] = 0.0;
	energy["Transmission_STDP"] = 0.0;

	Json &util = energy_model["util"];
	util["non_spiking_rec"]["number_of_neurons"] = 0.0;
	util["non_spiking_non_rec"]["number_of_neurons"] = 0.0;
	util["full_spiking_rec"]["number_of_neurons"] = 0.0;
	util["full_spiking_rec"]["number_of_spikes"] = 0.0;
	util["full_spiking_rec"]["runtime"] = 0.0;

	util["full_spiking_non_rec"]["number_of_neurons"] = 0.0;
	util["full_spiking_non_rec"]["runtime"] = 0.0;
	util["full_spiking_non_rec"]["number_of_spikes"] = 0.0;

	util["input_O2O"]["number_of_neurons"] = 0.0;
	util["input_O2O"]["runtime"] = 0.0;
	util["input_O2O"]["number_of_spikes"] = 0.0;

	util["input_A2A"]["number_of_neurons"] = 0.0;
	util["input_A2A"]["runtime"] = 0.0;
	util["input_A2A"]["number_of_spikes"] = 0.0;

	util["inter_s2A"]["number_of_neurons"] = 0.0;
	util["inter_s2A"]["runtime"] = 0.0;
	util["inter_s2A"]["number_of_spikes"] = 0.0;
	util["inter_O2O"]["number_of_neurons"] = 0.0;
	util["inter_O2O"]["runtime"] = 0.0;
	util["inter_O2O"]["number_of_spikes"] = 0.0;

	util["stdp_idle"]["number_of_neurons"] = 0.0;
	util["stdp_spike"]["number_of_neurons"] = 0.0;
	util["stdp_spike"]["runtime"] = 0.0;
	util["stdp_spike"]["number_of_spikes"] = 0.0;
	util["stdp_spike"]["number_of_source_spikes"] = 0.0;

	return energy_model;
}

void calculate_coefficients(Json &energy_model, size_t repeat)
{
	Json &measured = energy_model["measured"];
	Json &power = energy_model["power"];
	Json &energy = energy_model["energy"];
	Json &util = energy_model["util"];

	normalize2(util, double(repeat));
	normalize(measured, double(repeat));
	// Ugly hack, as it is only recorded once
	measured["pre_boot"] = measured["pre_boot"].get<double>() * double(repeat);

	power["pre_boot"] =
	    measured["pre_boot"].get<double>() / 1000.0;  // Joule per microsecond

	power["idle"] = measured["idle"].get<double>() / 1000.0;

	// Consumption per neuron per
	power["idle_recorded_neurons"] =
	    (measured["non_spiking_rec"].get<double>() / 1000.0 -
	     power["idle"].get<double>()) /
	    util["non_spiking_rec"]["number_of_neurons"].get<double>();
	power["idle_neurons"] =
	    (measured["non_spiking_non_rec"].get<double>() / 1000.0 -
	     power["idle"].get<double>()) /
	    util["non_spiking_non_rec"]["number_of_neurons"].get<double>();
	energy["spike"] =
	    (measured["full_spiking_rec"].get<double>() / 1000.0 -
	     (power["idle_recorded_neurons"].get<double>() *
	      util["full_spiking_rec"]["number_of_neurons"].get<double>()) -
	     power["idle"].get<double>()) *
	    util["full_spiking_rec"]["runtime"].get<double>() /
	    util["full_spiking_rec"]["number_of_spikes"]
	        .get<double>();  // Energy of a recorded spike

	energy["not_recorded_spike"] =
	    (measured["full_spiking_non_rec"].get<double>() / 1000.0 -
	     power["idle"].get<double>() -
	     (power["idle_neurons"].get<double>() *
	      util["full_spiking_non_rec"]["number_of_neurons"].get<double>())) *
	    util["full_spiking_non_rec"]["runtime"].get<double>() /
	    util["full_spiking_non_rec"]["number_of_spikes"]
	        .get<double>();  // Energy of a not recorded spike

	energy["InputSpike_O2O"] =
	    (measured["input_O2O"].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"].get<double>() *
	         util["input_O2O"]["number_of_neurons"].get<double>() -
	     power["idle"].get<double>()) *
	    util["input_O2O"]["runtime"].get<double>() /
	    util["input_O2O"]["number_of_spikes"]
	        .get<double>();  // Energy per input spike

	energy["InputSpike_A2A"] =
	    (measured["input_A2A"].get<double>() / 1000.0 -
	     (power["idle_recorded_neurons"].get<double>() *
	      util["input_A2A"]["number_of_neurons"].get<double>()) -
	     power["idle"].get<double>()) *
	    util["input_A2A"]["runtime"].get<double>() /
	    (util["input_A2A"]["number_of_spikes"].get<double>() *
	     util["input_A2A"]["number_of_neurons"]
	         .get<double>());  // Energy per input spike

	energy["Transmission_S2A"] =
	    ((measured["inter_s2A"].get<double>() / 1000.0 -
	      power["idle_recorded_neurons"].get<double>() *
	          double(util["inter_s2A"]["number_of_neurons"].get<double>() +
	                 1.0) -
	      power["idle"].get<double>()) *
	         util["inter_s2A"]["runtime"].get<double>() -
	     energy["spike"].get<double>() *
	         util["inter_s2A"]["number_of_spikes"].get<double>()) /
	    (util["inter_s2A"]["number_of_spikes"].get<double>() *
	     util["inter_s2A"]["number_of_neurons"]
	         .get<double>());  // E of pre-syn spike

	energy["Transmission_O2O"] =
	    ((measured["inter_O2O"].get<double>() / 1000.0 -
	      power["idle_recorded_neurons"].get<double>() *
	          double(2 * util["inter_O2O"]["number_of_neurons"].get<double>()) -
	      power["idle"].get<double>()) *
	         util["inter_O2O"]["runtime"].get<double>() -
	     energy["spike"].get<double>() *
	         util["inter_O2O"]["number_of_spikes"].get<double>()) /
	    util["inter_O2O"]["number_of_spikes"]
	        .get<double>();  // E of pre-syn spike

	if (energy_model["stdp"].get<bool>()) {
		power["idle_stdp"] =
		    (measured["stdp_idle"].get<double>() / 1000.0 -
		     util["stdp_idle"]["number_of_neurons"].get<double>() *
		         power["idle_recorded_neurons"].get<double>() -
		     power["idle"].get<double>()) /
		    util["stdp_idle"]["number_of_neurons"]
		        .get<double>();  // Power of idle STDP synapse //

		energy["Transmission_STDP"] =
		    ((measured["stdp_spike"].get<double>() / 1000.0 -
		      power["idle"].get<double>() -
		      util["stdp_spike"]["number_of_neurons"].get<double>() *
		          power["idle_stdp"].get<double>() -
		      util["stdp_spike"]["number_of_neurons"].get<double>() *
		          power["idle_recorded_neurons"].get<double>()) *
		         util["stdp_spike"]["runtime"].get<double>() -
		     energy["spike"].get<double>() *
		         util["stdp_spike"]["number_of_spikes"].get<double>() -
		     util["stdp_spike"]["number_of_source_spikes"].get<double>() *
		         energy["InputSpike_O2O"].get<double>()) /
		    util["stdp_spike"]["number_of_source_spikes"].get<double>();
		// E per spike transmitted via STDP synapse
	}
}
