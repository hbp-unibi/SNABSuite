
#include "energy_utils.hpp"

#include <cypress/cypress.hpp>

namespace Energy {
using namespace cypress;

size_t get_number_of_spikes_pop(const PopulationBase &pop)
{
	size_t spikes = 0;
	if (pop.signals().is_recording(0)) {
		for (const auto &neuron : pop) {
			spikes += neuron.signals().data(0).size();
		}
	}
	return spikes;
}

size_t get_number_of_spikes(Network &netw, bool sources)
{
	size_t spikes = 0;
	for (const auto &pop : netw.populations()) {
		if (&pop.type() == &SpikeSourceArray::inst() && !sources) {
			continue;
		}
		spikes += get_number_of_spikes_pop(pop);
	}
	return spikes;
}

size_t get_number_of_neurons(cypress::Network &netw, bool sources)
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

size_t calc_number_stdp_synapses(const cypress::Network &netw)
{

	size_t res = 0;
	auto &conns = netw.connections();
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
				for (auto lc : connections) {
					if (lc.valid()) {
						res++;
					}
				}
			}
		}
	}
	return res;
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

std::tuple<size_t, size_t, size_t> calc_postsyn_spikes(
    const cypress::PopulationBase &pop,
    const std::vector<cypress::ConnectionDescriptor> &conns, bool stdp)
{
	auto conn_ids = conn_ids_source(pop.pid(), conns);
	size_t spikes_one = 0, spikes_all = 0, spikes_misc = 0;
	const auto &netw = pop.network();
	for (auto cid : conn_ids) {
		auto name = conns[cid].connector().name();
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
				spikes_one += neuron.signals().data(0).size();
			}
		}
		else if (name == "FixedFanOutConnector") {
			size_t fan_out =
			    size_t(conns[cid].connector().additional_parameter());
			for (const auto &neuron : pop) {
				spikes_misc += neuron.signals().data(0).size() * fan_out;
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
				if (lc.valid()) {
					spikes_misc += pop[lc.src].signals().data(0).size();
				}
			}
		}
	}
	return std::tuple<size_t, size_t, size_t>(spikes_one, spikes_all,
	                                          spikes_misc);
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
	measured["input_random"] = 0.0;
	energy["InputSpike_random"] = 0.0;
	measured["inter_s2A"] = 0.0;
	energy["Transmission_S2A"] = 0.0;
	measured["inter_O2O"] = 0.0;
	energy["Transmission_O2O"] = 0.0;
	measured["inter_random"] = 0.0;
	energy["Transmission_random"] = 0.0;
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

	util["input_random"]["number_of_neurons"] = 0.0;
	util["input_random"]["runtime"] = 0.0;
	util["input_random"]["number_of_spikes"] = 0.0;
	util["input_random"]["fan_out"] = 0.0;

	util["inter_s2A"]["number_of_neurons"] = 0.0;
	util["inter_s2A"]["runtime"] = 0.0;
	util["inter_s2A"]["number_of_spikes"] = 0.0;
	util["inter_O2O"]["number_of_neurons"] = 0.0;
	util["inter_O2O"]["runtime"] = 0.0;
	util["inter_O2O"]["number_of_spikes"] = 0.0;
	util["inter_random"]["number_of_neurons"] = 0.0;
	util["inter_random"]["runtime"] = 0.0;
	util["inter_random"]["number_of_spikes"] = 0.0;
	util["inter_random"]["connections"] = 0.0;

	util["stdp_idle"]["number_of_neurons"] = 0.0;
	util["stdp_spike"]["number_of_neurons"] = 0.0;
	util["stdp_spike"]["runtime"] = 0.0;
	util["stdp_spike"]["number_of_spikes"] = 0.0;
	util["stdp_spike"]["number_of_source_spikes"] = 0.0;

	return energy_model;
}
namespace{
/**
 * @brief Normalize values in a Json 1d
 *
 * @param json target json object
 * @param norm the value to devide by
 */
void normalize(Json &json, double norm)
{
	for (auto &i : json.items()) {
		i.value() = i.value().get<double>() / norm;
	}
}

/**
 * @brief Normalize values in a Json 2d
 *
 * @param json target json object
 * @param norm the value to devide by
 */
void normalize2(Json &json, double norm)
{
	for (auto &i : json.items()) {
		for (auto &j : i.value().items()) {
			j.value() = j.value().get<double>() / norm;
		}
	}
}
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

	energy["InputSpike_random"] =
	    (measured["input_random"].get<double>() / 1000.0 -
	     (power["idle_recorded_neurons"].get<double>() *
	      util["input_A2A"]["number_of_neurons"].get<double>()) -
	     power["idle"].get<double>()) *
	    util["input_random"]["runtime"].get<double>() /
	    (util["input_random"]["number_of_spikes"].get<double>() *
	     util["input_random"]["fan_out"]
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

	energy["Transmission_random"] =
	    ((measured["inter_random"].get<double>() / 1000.0 -
	      power["idle_recorded_neurons"].get<double>() *
	          double(util["inter_random"]["number_of_neurons"].get<double>()) -
	      power["idle"].get<double>()) *
	         util["inter_random"]["runtime"].get<double>() -
	     energy["spike"].get<double>() *
	         util["inter_random"]["number_of_spikes"].get<double>()) /
	    (util["inter_random"]["number_of_spikes"].get<double>() *
	     util["inter_random"]["connections"]
	         .get<double>());  // E of pre-syn spike

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
		         energy["InputSpike_A2A"].get<double>() *
		         util["stdp_spike"]["number_of_neurons"].get<double>()) /
		    (util["stdp_spike"]["number_of_source_spikes"].get<double>() *
		     util["stdp_spike"]["number_of_neurons"].get<double>());
		// E per spike transmitted via STDP synapse
	}
}

double calculate_energy(const cypress::Network &netw, const Json &energy_model)
{
	double runtime = netw.runtime().sim_pure * 1000.0;
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
			energy += double(std::get<2>(spikes)) *
			          energy_model["energy"]["InputSpike_random"].get<double>();
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
			energy +=
			    double(std::get<2>(spikes2)) *
			    energy_model["energy"]["Transmission_random"].get<double>();
		}
	}

	if (energy_model["stdp"].get<bool>()) {
		energy += double(calc_number_stdp_synapses(netw)) *
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
				energy +=
				    double(std::get<2>(spikes2)) *
				    energy_model["energy"]["Transmission_STDP"].get<double>();
			}
		}
	}
	return energy;
}

}  // namespace Energy
