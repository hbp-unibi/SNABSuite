
#include "energy_utils.hpp"

#include <cypress/cypress.hpp>

#include "util/utilities.hpp"

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
	measured["pre_boot"] = 0.0;
	measured["non_spiking_rec"] = std::vector<double>();
	measured["idle"] = std::vector<double>();
	measured["non_spiking_non_rec"] = std::vector<double>();
	measured["full_spiking_rec"] = std::vector<double>();
	measured["full_spiking_non_rec"] = std::vector<double>();
	measured["input_O2O"] = std::vector<double>();
	measured["input_A2A"] = std::vector<double>();
	measured["input_random"] = std::vector<double>();
	measured["inter_s2A"] = std::vector<double>();
	measured["inter_O2O"] = std::vector<double>();
	measured["inter_random"] = std::vector<double>();
	measured["stdp_idle"] = std::vector<double>();
	measured["stdp_spike"] = std::vector<double>();

	Json &util = energy_model["util"];
	util["non_spiking_rec"]["number_of_neurons"] = std::vector<double>();
	util["non_spiking_rec"]["runtime"] = std::vector<double>();
	util["non_spiking_rec"]["bioruntime"] = 0.0;
	util["non_spiking_non_rec"]["number_of_neurons"] = std::vector<double>();
	util["non_spiking_non_rec"]["runtime"] = std::vector<double>();
	util["non_spiking_non_rec"]["bioruntime"] = 0.0;
	util["full_spiking_rec"]["number_of_neurons"] = std::vector<double>();
	util["full_spiking_rec"]["number_of_spikes"] = std::vector<double>();
	util["full_spiking_rec"]["runtime"] = std::vector<double>();
	util["full_spiking_rec"]["bioruntime"] = 0.0;

	util["full_spiking_non_rec"]["number_of_neurons"] = std::vector<double>();
	util["full_spiking_non_rec"]["runtime"] = std::vector<double>();
	util["full_spiking_non_rec"]["bioruntime"] = 0.0;
	util["full_spiking_non_rec"]["number_of_spikes"] = std::vector<double>();

	util["input_O2O"]["number_of_neurons"] = std::vector<double>();
	util["input_O2O"]["runtime"] = std::vector<double>();
	util["input_O2O"]["bioruntime"] = 0.0;
	util["input_O2O"]["number_of_spikes"] = std::vector<double>();
	util["input_O2O"]["number_of_spikes_tar"] = std::vector<double>();

	util["input_A2A"]["number_of_neurons"] = std::vector<double>();
	util["input_A2A"]["runtime"] = std::vector<double>();
	util["input_A2A"]["bioruntime"] = 0.0;
	util["input_A2A"]["number_of_spikes"] = std::vector<double>();
	util["input_A2A"]["number_of_spikes_tar"] = std::vector<double>();

	util["input_random"]["number_of_neurons"] = std::vector<double>();
	util["input_random"]["runtime"] = std::vector<double>();
	util["input_random"]["bioruntime"] = 0.0;
	util["input_random"]["number_of_spikes"] = std::vector<double>();
	util["input_random"]["fan_out"] = std::vector<double>();
	util["input_random"]["number_of_spikes_tar"] = std::vector<double>();

	util["inter_s2A"]["number_of_neurons"] = std::vector<double>();
	util["inter_s2A"]["runtime"] = std::vector<double>();
	util["inter_s2A"]["bioruntime"] = 0.0;
	util["inter_s2A"]["number_of_spikes"] = std::vector<double>();
	util["inter_s2A"]["number_of_spikes_tar"] = std::vector<double>();
	util["inter_O2O"]["number_of_neurons"] = std::vector<double>();
	util["inter_O2O"]["runtime"] = std::vector<double>();
	util["inter_O2O"]["bioruntime"] = 0.0;
	util["inter_O2O"]["number_of_spikes"] = std::vector<double>();
	util["inter_O2O"]["number_of_spikes_tar"] = std::vector<double>();
	util["inter_random"]["number_of_neurons"] = std::vector<double>();
	util["inter_random"]["runtime"] = std::vector<double>();
	util["inter_random"]["bioruntime"] = 0.0;
	util["inter_random"]["number_of_spikes"] = std::vector<double>();
	util["inter_random"]["number_of_spikes_tar"] = std::vector<double>();
	util["inter_random"]["connections"] = std::vector<double>();

	util["stdp_idle"]["number_of_neurons"] = std::vector<double>();
	util["stdp_idle"]["runtime"] = std::vector<double>();
	util["stdp_idle"]["bioruntime"] = 0.0;
	util["stdp_spike"]["number_of_neurons"] = std::vector<double>();
	util["stdp_spike"]["runtime"] = std::vector<double>();
	util["stdp_spike"]["bioruntime"] = 0.0;
	util["stdp_spike"]["number_of_spikes"] = std::vector<double>();
	util["stdp_spike"]["number_of_source_spikes"] = std::vector<double>();

	return energy_model;
}
namespace {

void calculate_statistics(Json &json)
{
	Json res = json;
	for (auto entry = json.begin(); entry != json.end(); entry++) {
		if (entry.value().is_array()) {
			double min, max, avg, std_dev;
			SNAB::Utilities::calculate_statistics(
			    entry.value().get<std::vector<double>>(), min, max, avg,
			    std_dev);
			res[entry.key() + "_avg"] = std::vector<double>{avg, std_dev};
		}
		else if (entry.value().is_object()) {
			calculate_statistics(res[entry.key()]);
		}
	}
	json = res;
}
double error_multiply(const Json &a, const Json &b)
{
	double res = a[1].get<double>() / a[0].get<double>();
	res += b[1].get<double>() / b[0].get<double>();
	res *= a[0].get<double>() * b[0].get<double>();
	return res;
}

double error_multiply(const double a, const double a_err, const Json &b)
{
	double res = a_err / a;
	res += b[1].get<double>() / b[0].get<double>();
	res *= a * b[0].get<double>();
	return res;
}

double error_devide(const double dividend, const double err_dividend,
                    const Json &b)
{
	double res = err_dividend / dividend;
	res += b[1].get<double>() /
	       b[0].get<double>();  // Negative sign if signed errors only
	res *= dividend / b[0].get<double>();
	return res;
}

double error_devide(const double dividend, const double err_dividend,
                    const double divisor, const double err_divisor)
{
	double res =
	    (err_dividend / dividend) +
	    (err_divisor / divisor);  // Negative sign if signed errors only
	res *= dividend / divisor;
	return res;
}

}  // namespace

void calc_coef_norm(Json &energy_model)
{
	Json &measured = energy_model["measured"];
	Json &power = energy_model["power"];
	Json &energy = energy_model["energy"];
	Json &util = energy_model["util"];

	double val, error;
	// Consumption per neuron per
	val = (measured["non_spiking_rec_avg"][0].get<double>() / 1000.0 -
	       power["idle"][0].get<double>()) /
	      util["non_spiking_rec"]["number_of_neurons_avg"][0].get<double>();
	error = (measured["non_spiking_rec_avg"][1].get<double>() / 1000.0 +
	         power["idle"][1].get<double>()) /
	        (measured["non_spiking_rec_avg"][0].get<double>() / 1000.0 -
	         power["idle"][0].get<double>());  // relative error
	error +=
	    (util["non_spiking_rec"]["number_of_neurons_avg"][1].get<double>() /
	     util["non_spiking_rec"]["number_of_neurons_avg"][0]
	         .get<double>());  // relative error
	error *= val;

	error = error_multiply(val, error, util["non_spiking_rec"]["runtime_avg"]) /
	        util["non_spiking_rec"]["bioruntime"].get<double>();
	val *= util["non_spiking_rec"]["runtime_avg"][0].get<double>() /
	       util["non_spiking_rec"]["bioruntime"].get<double>();
	energy["idle_recorded_neurons_ms"] = {val, error};

	val = (measured["non_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	       power["idle"][0].get<double>()) /
	      util["non_spiking_non_rec"]["number_of_neurons_avg"][0].get<double>();

	error = (measured["non_spiking_non_rec_avg"][1].get<double>() / 1000.0 +
	         power["idle"][1].get<double>()) /
	        (measured["non_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	         power["idle"][0].get<double>());
	error +=
	    (util["non_spiking_non_rec"]["number_of_neurons_avg"][1].get<double>() /
	     util["non_spiking_non_rec"]["number_of_neurons_avg"][0].get<double>());
	error *= val;

	error =
	    error_multiply(val, error, util["non_spiking_non_rec"]["runtime_avg"]) /
	    util["non_spiking_non_rec"]["bioruntime"].get<double>();
	val *= util["non_spiking_non_rec"]["runtime_avg"][0].get<double>() /
	       util["non_spiking_non_rec"]["bioruntime"].get<double>();
	energy["idle_neurons_ms"] = {val, error};

	// Energy of a recorded spike
	double dividend =
	    (measured["full_spiking_rec_avg"][0].get<double>() / 1000.0 -
	     power["idle"][0].get<double>());
	double error_dividend =
	    measured["full_spiking_rec_avg"][1].get<double>() / 1000.0;
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["full_spiking_rec"]["runtime_avg"]);
	dividend *= util["full_spiking_rec"]["runtime_avg"][0].get<double>();
	dividend -=
	    energy["idle_recorded_neurons_ms"][0].get<double>() *
	    util["full_spiking_rec"]["number_of_neurons_avg"][0].get<double>() *
	    util["full_spiking_rec"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_recorded_neurons_ms"],
	                   util["full_spiking_rec"]["number_of_neurons_avg"]) *
	    util["full_spiking_rec"]["bioruntime"].get<double>();
	val = dividend /
	      util["full_spiking_rec"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["full_spiking_rec"]["number_of_spikes_avg"]);
	energy["spike"] = {val, error};

	// Energy of a not recorded spike
	dividend = (measured["full_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	error_dividend =
	    measured["full_spiking_non_rec_avg"][1].get<double>() / 1000.0 +
	    power["idle"][1].get<double>();
	error_dividend = error_multiply(
	    dividend, error_dividend, util["full_spiking_non_rec"]["runtime_avg"]);
	dividend *= util["full_spiking_non_rec"]["runtime_avg"][0].get<double>();
	dividend -=
	    energy["idle_neurons_ms"][0].get<double>() *
	    util["full_spiking_non_rec"]["number_of_neurons_avg"][0].get<double>() *
	    util["full_spiking_non_rec"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_neurons_ms"],
	                   util["full_spiking_non_rec"]["number_of_neurons_avg"]) *
	    util["full_spiking_non_rec"]["bioruntime"].get<double>();
	val = dividend /
	      util["full_spiking_non_rec"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["full_spiking_non_rec"]["number_of_spikes_avg"]);
	energy["not_recorded_spike"] = {val, error};

	// Energy per input spike
	dividend = (measured["input_O2O_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	error_dividend = (measured["input_O2O_avg"][1].get<double>() / 1000.0 +
	                  power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_O2O"]["runtime_avg"]);
	dividend *= util["input_O2O"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            util["input_O2O"]["number_of_neurons_avg"][0].get<double>() *
	            util["input_O2O"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_recorded_neurons_ms"],
	                   util["input_O2O"]["number_of_neurons_avg"]) *
	    util["input_O2O"]["bioruntime"].get<double>();
	dividend -= util["input_O2O"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_O2O"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();

	val = dividend / util["input_O2O"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["input_O2O"]["number_of_spikes_avg"]);
	energy["InputSpike_O2O"] = {val, error};

	// Energy per input spike
	dividend = (measured["input_A2A_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	error_dividend = (measured["input_A2A_avg"][1].get<double>() / 1000.0 +
	                  power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_A2A"]["runtime_avg"]);
	dividend *= util["input_A2A"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            util["input_A2A"]["number_of_neurons_avg"][0].get<double>() *
	            util["input_A2A"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_recorded_neurons_ms"],
	                   util["input_A2A"]["number_of_neurons_avg"]) *
	    util["input_A2A"]["bioruntime"].get<double>();
	dividend -= util["input_A2A"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_A2A"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	double divisor =
	    util["input_A2A"]["number_of_spikes_avg"][0].get<double>() *
	    util["input_A2A"]["number_of_neurons_avg"][0].get<double>();
	double error_divisor =
	    error_multiply(util["input_A2A"]["number_of_spikes_avg"],
	                   util["input_A2A"]["number_of_neurons_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["InputSpike_A2A"] = {val, error};

	// Energy per input spike
	dividend = (measured["input_random_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	error_dividend = (measured["input_random_avg"][1].get<double>() / 1000.0 +
	                  power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_random"]["runtime_avg"]);
	dividend *= util["input_random"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            util["input_random"]["number_of_neurons_avg"][0].get<double>() *
	            util["input_random"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_recorded_neurons_ms"],
	                   util["input_random"]["number_of_neurons_avg"]) *
	    util["input_random"]["bioruntime"].get<double>();
	dividend -=
	    util["input_random"]["number_of_spikes_tar_avg"][0].get<double>() *
	    energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_random"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = util["input_random"]["number_of_spikes_avg"][0].get<double>() *
	          util["input_random"]["fan_out_avg"][0].get<double>();
	error_divisor = error_multiply(util["input_random"]["number_of_spikes_avg"],
	                               util["input_random"]["fan_out_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["InputSpike_random"] = {val, error};

	// E of pre-syn spike
	dividend = (measured["inter_s2A_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	auto tmp = util["inter_s2A"]["number_of_neurons_avg"];
	tmp[0] = tmp[0].get<double>() + 1.0;
	error_dividend = measured["inter_s2A_avg"][1].get<double>() / 1000.0 +
	                 power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_s2A"]["runtime_avg"]);
	dividend *= util["inter_s2A"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            tmp[0].get<double>() *
	            util["inter_s2A"]["bioruntime"].get<double>();
	error_dividend += error_multiply(energy["idle_recorded_neurons_ms"], tmp) *
	                  util["inter_s2A"]["bioruntime"].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_s2A"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(energy["spike"],
	                                 util["inter_s2A"]["number_of_spikes_avg"]);
	dividend -= util["inter_s2A"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_s2A"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_s2A"]["number_of_spikes_avg"][0].get<double>() *
	           util["inter_s2A"]["number_of_neurons_avg"][0].get<double>());
	error_divisor = error_multiply(util["inter_s2A"]["number_of_spikes_avg"],
	                               util["inter_s2A"]["number_of_neurons_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_S2A"] = {val, error};

	// E of pre-syn spike
	dividend = (measured["inter_O2O_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	tmp = util["inter_O2O"]["number_of_neurons_avg"];
	tmp[0] = 2.0 * tmp[0].get<double>();
	tmp[1] = 2.0 * tmp[1].get<double>();
	error_dividend = measured["inter_O2O_avg"][1].get<double>() / 1000.0 +
	                 power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_O2O"]["runtime_avg"]);
	dividend *= util["inter_O2O"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            tmp[0].get<double>() *
	            util["inter_O2O"]["bioruntime"].get<double>();
	error_dividend += error_multiply(energy["idle_recorded_neurons_ms"], tmp) *
	                  util["inter_O2O"]["bioruntime"].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_O2O"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(energy["spike"],
	                                 util["inter_O2O"]["number_of_spikes_avg"]);
	dividend -= util["inter_O2O"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_O2O"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_O2O"]["number_of_spikes_avg"][0].get<double>());
	error_divisor = util["inter_O2O"]["number_of_spikes_avg"][1].get<double>();
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_O2O"] = {val, error};

	dividend = (measured["inter_random_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>());
	error_dividend = measured["inter_random_avg"][1].get<double>() / 1000.0 +
	                 power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_random"]["runtime_avg"]);
	dividend *= util["inter_random"]["runtime_avg"][0].get<double>();
	dividend -= energy["idle_recorded_neurons_ms"][0].get<double>() *
	            util["inter_random"]["number_of_neurons_avg"][0].get<double>() *
	            util["inter_random"]["bioruntime"].get<double>();
	error_dividend +=
	    error_multiply(energy["idle_recorded_neurons_ms"],
	                   util["inter_random"]["number_of_neurons_avg"]) *
	    util["inter_random"]["bioruntime"].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_random"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(
	    energy["spike"], util["inter_random"]["number_of_spikes_avg"]);
	dividend -=
	    util["inter_random"]["number_of_spikes_tar_avg"][0].get<double>() *
	    energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_random"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_random"]["number_of_spikes_avg"][0].get<double>() *
	           util["inter_random"]["connections_avg"][0].get<double>());
	error_divisor = error_multiply(util["inter_random"]["number_of_spikes_avg"],
	                               util["inter_random"]["connections_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_random"] = {val, error};

	if (energy_model["stdp"].get<bool>()) {
		// Power of idle STDP synapse //
		dividend = (measured["stdp_idle_avg"][0].get<double>() / 1000.0 -
		            power["idle"][0].get<double>());
		error_dividend = (measured["stdp_idle_avg"][1].get<double>() / 1000.0 +
		                  power["idle"][1].get<double>());
		error_dividend = error_multiply(dividend, error_dividend,
		                                util["stdp_idle"]["runtime_avg"]);
		dividend *= util["stdp_idle"]["runtime_avg"][0].get<double>();

		dividend -=
		    util["stdp_idle"]["number_of_neurons_avg"][0].get<double>() *
		    energy["idle_recorded_neurons_ms"][0].get<double>();
		error_dividend +=
		    error_multiply(util["stdp_idle"]["number_of_neurons_avg"],
		                   energy["idle_recorded_neurons_ms"]);

		divisor = util["stdp_idle"]["number_of_neurons_avg"][0];
		error_divisor = util["stdp_idle"]["number_of_neurons_avg"][1];
		val = dividend / divisor;
		error = error_devide(dividend, error_dividend, divisor, error_divisor);
		auto bioruntime = util["stdp_idle"]["bioruntime"].get<double>();
		energy["idle_stdp_ms"] = {val / bioruntime, error / bioruntime};

		// E per spike transmitted via STDP synapse
		dividend = (measured["stdp_spike_avg"][0].get<double>() / 1000.0 -
		            power["idle"][0].get<double>());
		error_dividend = (measured["stdp_spike_avg"][1].get<double>() / 1000.0 +
		                  power["idle"][1].get<double>());
		error_dividend = error_multiply(dividend, error_dividend,
		                                util["stdp_spike"]["runtime_avg"]);
		dividend *= util["stdp_spike"]["runtime_avg"][0].get<double>();
		dividend -=
		    energy["idle_recorded_neurons_ms"][0].get<double>() *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>() *
		    util["stdp_spike"]["bioruntime"].get<double>();
		error_dividend +=
		    error_multiply(energy["idle_recorded_neurons_ms"],
		                   util["stdp_spike"]["number_of_neurons_avg"]) *
		    util["stdp_spike"]["bioruntime"].get<double>();
		dividend -=
		    energy["idle_stdp_ms"][0].get<double>() *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>() *
		    util["stdp_spike"]["bioruntime"].get<double>();
		error_dividend +=
		    error_multiply(energy["idle_stdp_ms"],
		                   util["stdp_spike"]["number_of_neurons_avg"]) *
		    util["stdp_spike"]["bioruntime"].get<double>();
		dividend -= energy["spike"][0].get<double>() *
		            util["stdp_spike"]["number_of_spikes_avg"][0].get<double>();
		error_dividend += error_multiply(
		    energy["spike"], util["stdp_spike"]["number_of_spikes_avg"]);
		double first =
		    util["stdp_spike"]["number_of_source_spikes_avg"][0].get<double>() *
		    energy["InputSpike_A2A"][0].get<double>();
		double error_first =
		    error_multiply(util["stdp_spike"]["number_of_source_spikes_avg"],
		                   energy["InputSpike_A2A"]);

		error_dividend += error_multiply(
		    first, error_first, util["stdp_spike"]["number_of_neurons_avg"]);
		dividend -=
		    first *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>();
		divisor =
		    util["stdp_spike"]["number_of_source_spikes_avg"][0].get<double>() *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>();
		error_divisor =
		    error_multiply(util["stdp_spike"]["number_of_source_spikes_avg"],
		                   util["stdp_spike"]["number_of_neurons_avg"]);
		val = dividend / divisor;
		error = error_devide(dividend, error_dividend, divisor, error_divisor);
		energy["Transmission_STDP"] = {val, error};
	}
}

void calculate_coefficients(Json &energy_model)
{
	Json &measured = energy_model["measured"];
	Json &power = energy_model["power"];
	Json &energy = energy_model["energy"];
	Json &util = energy_model["util"];
	calculate_statistics(measured);
	calculate_statistics(util);

	power["pre_boot"] = measured["pre_boot"].get<double>() /
	                    1000.0;  // Watt to kWatt (multiply with runtime in ms)

	power["idle"] = {
	    measured["idle_avg"][0].get<double>() / 1000.0,
	    measured["idle_avg"][1].get<double>() /
	        1000.0};  // Watt to kWatt (multiply with runtime in ms)

	if (energy_model.find("runtime_normalized") != energy_model.end() &&
	    energy_model["runtime_normalized"].get<bool>()) {
		calc_coef_norm(energy_model);
		return;
	}
	double val, error;
	// Consumption per neuron per
	val = (measured["non_spiking_rec_avg"][0].get<double>() / 1000.0 -
	       power["idle"][0].get<double>()) /
	      util["non_spiking_rec"]["number_of_neurons_avg"][0].get<double>();
	error = (measured["non_spiking_rec_avg"][1].get<double>() / 1000.0 +
	         power["idle"][1].get<double>()) /
	        (measured["non_spiking_rec_avg"][0].get<double>() / 1000.0 -
	         power["idle"][0].get<double>());  // relative error
	error +=
	    (util["non_spiking_rec"]["number_of_neurons_avg"][1].get<double>() /
	     util["non_spiking_rec"]["number_of_neurons_avg"][0]
	         .get<double>());  // relative error
	error *= val;
	power["idle_recorded_neurons"] = {val, error};

	val = (measured["non_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	       power["idle"][0].get<double>()) /
	      util["non_spiking_non_rec"]["number_of_neurons_avg"][0].get<double>();

	error = (measured["non_spiking_non_rec_avg"][1].get<double>() / 1000.0 +
	         power["idle"][1].get<double>()) /
	        (measured["non_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	         power["idle"][0].get<double>());
	error +=
	    (util["non_spiking_non_rec"]["number_of_neurons_avg"][1].get<double>() /
	     util["non_spiking_non_rec"]["number_of_neurons_avg"][0].get<double>());
	error *= val;
	power["idle_neurons"] = {val, error};

	// Energy of a recorded spike
	double dividend =
	    (measured["full_spiking_rec_avg"][0].get<double>() / 1000.0 -
	     (power["idle_recorded_neurons"][0].get<double>() *
	      util["full_spiking_rec"]["number_of_neurons_avg"][0].get<double>()) -
	     power["idle"][0].get<double>());
	double error_dividend =
	    measured["full_spiking_rec_avg"][1].get<double>() / 1000.0 +
	    error_multiply(util["full_spiking_rec"]["number_of_neurons_avg"],
	                   power["idle_recorded_neurons"]);
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["full_spiking_rec"]["runtime_avg"]);
	dividend *= util["full_spiking_rec"]["runtime_avg"][0].get<double>();
	val = dividend /
	      util["full_spiking_rec"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["full_spiking_rec"]["number_of_spikes_avg"]);
	energy["spike"] = {val, error};

	// Energy of a not recorded spike
	dividend = (measured["full_spiking_non_rec_avg"][0].get<double>() / 1000.0 -
	            power["idle"][0].get<double>() -
	            (power["idle_neurons"][0].get<double>() *
	             util["full_spiking_non_rec"]["number_of_neurons_avg"][0]
	                 .get<double>()));
	error_dividend =
	    measured["full_spiking_non_rec_avg"][1].get<double>() / 1000.0 +
	    power["idle"][1].get<double>() +
	    error_multiply(power["idle_neurons"],
	                   util["full_spiking_non_rec"]["number_of_neurons_avg"]);
	error_dividend = error_multiply(
	    dividend, error_dividend, util["full_spiking_non_rec"]["runtime_avg"]);
	dividend *= util["full_spiking_non_rec"]["runtime_avg"][0].get<double>();
	val = dividend /
	      util["full_spiking_non_rec"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["full_spiking_non_rec"]["number_of_spikes_avg"]);
	energy["not_recorded_spike"] = {val, error};

	// Energy per input spike
	dividend =
	    (measured["input_O2O_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         util["input_O2O"]["number_of_neurons_avg"][0].get<double>() -
	     power["idle"][0].get<double>());
	error_dividend =
	    (measured["input_O2O_avg"][1].get<double>() / 1000.0 +
	     error_multiply(power["idle_recorded_neurons"],
	                    util["input_O2O"]["number_of_neurons_avg"]) +
	     power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_O2O"]["runtime_avg"]);
	dividend *= util["input_O2O"]["runtime_avg"][0].get<double>();
	dividend -= util["input_O2O"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_O2O"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	val = dividend / util["input_O2O"]["number_of_spikes_avg"][0].get<double>();
	error = error_devide(dividend, error_dividend,
	                     util["input_O2O"]["number_of_spikes_avg"]);
	energy["InputSpike_O2O"] = {val, error};

	// Energy per input spike
	dividend =
	    (measured["input_A2A_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         util["input_A2A"]["number_of_neurons_avg"][0].get<double>() -
	     power["idle"][0].get<double>());
	error_dividend =
	    (measured["input_A2A_avg"][1].get<double>() / 1000.0 +
	     error_multiply(power["idle_recorded_neurons"],
	                    util["input_A2A"]["number_of_neurons_avg"]) +
	     power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_A2A"]["runtime_avg"]);
	dividend *= util["input_A2A"]["runtime_avg"][0].get<double>();
	dividend -= util["input_A2A"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_A2A"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	double divisor =
	    util["input_A2A"]["number_of_spikes_avg"][0].get<double>() *
	    util["input_A2A"]["number_of_neurons_avg"][0].get<double>();
	double error_divisor =
	    error_multiply(util["input_A2A"]["number_of_spikes_avg"],
	                   util["input_A2A"]["number_of_neurons_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["InputSpike_A2A"] = {val, error};

	// Energy per input spike
	dividend =
	    (measured["input_random_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         util["input_random"]["number_of_neurons_avg"][0].get<double>() -
	     power["idle"][0].get<double>());
	error_dividend =
	    (measured["input_random_avg"][1].get<double>() / 1000.0 +
	     error_multiply(power["idle_recorded_neurons"],
	                    util["input_random"]["number_of_neurons_avg"]) +
	     power["idle"][1].get<double>());
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["input_random"]["runtime_avg"]);
	dividend *= util["input_random"]["runtime_avg"][0].get<double>();
	dividend -=
	    util["input_random"]["number_of_spikes_tar_avg"][0].get<double>() *
	    energy["spike"][0].get<double>();
	error_dividend +=
	    util["input_random"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = util["input_random"]["number_of_spikes_avg"][0].get<double>() *
	          util["input_random"]["fan_out_avg"][0].get<double>();
	error_divisor = error_multiply(util["input_random"]["number_of_spikes_avg"],
	                               util["input_random"]["fan_out_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["InputSpike_random"] = {val, error};

	// E of pre-syn spike
	dividend =
	    (measured["inter_s2A_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         double(
	             util["inter_s2A"]["number_of_neurons_avg"][0].get<double>() +
	             1.0) -
	     power["idle"][0].get<double>());
	auto tmp = util["inter_s2A"]["number_of_neurons_avg"];
	tmp[0] = tmp[0].get<double>() + 1.0;
	error_dividend = measured["inter_s2A_avg"][1].get<double>() / 1000.0 +
	                 error_multiply(power["idle_recorded_neurons"], tmp) +
	                 power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_s2A"]["runtime_avg"]);
	dividend *= util["inter_s2A"]["runtime_avg"][0].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_s2A"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(energy["spike"],
	                                 util["inter_s2A"]["number_of_spikes_avg"]);
	dividend -= util["inter_s2A"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_s2A"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_s2A"]["number_of_spikes_avg"][0].get<double>() *
	           util["inter_s2A"]["number_of_neurons_avg"][0].get<double>());
	error_divisor = error_multiply(util["inter_s2A"]["number_of_spikes_avg"],
	                               util["inter_s2A"]["number_of_neurons_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_S2A"] = {val, error};

	// E of pre-syn spike
	dividend =
	    (measured["inter_O2O_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         double(
	             2.0 *
	             util["inter_O2O"]["number_of_neurons_avg"][0].get<double>()) -
	     power["idle"][0].get<double>());
	tmp = util["inter_O2O"]["number_of_neurons_avg"];
	tmp[0] = 2.0 * tmp[0].get<double>();
	tmp[1] = 2.0 * tmp[1].get<double>();
	error_dividend = measured["inter_O2O_avg"][1].get<double>() / 1000.0 +
	                 error_multiply(power["idle_recorded_neurons"], tmp) +
	                 power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_O2O"]["runtime_avg"]);
	dividend *= util["inter_O2O"]["runtime_avg"][0].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_O2O"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(energy["spike"],
	                                 util["inter_O2O"]["number_of_spikes_avg"]);
	dividend -= util["inter_O2O"]["number_of_spikes_tar_avg"][0].get<double>() *
	            energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_O2O"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_O2O"]["number_of_spikes_avg"][0].get<double>());
	error_divisor = util["inter_O2O"]["number_of_spikes_avg"][1].get<double>();
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_O2O"] = {val, error};

	dividend =
	    (measured["inter_random_avg"][0].get<double>() / 1000.0 -
	     power["idle_recorded_neurons"][0].get<double>() *
	         util["inter_random"]["number_of_neurons_avg"][0].get<double>() -
	     power["idle"][0].get<double>());
	error_dividend =
	    measured["inter_random_avg"][1].get<double>() / 1000.0 +
	    error_multiply(power["idle_recorded_neurons"],
	                   util["inter_random"]["number_of_neurons_avg"]) +
	    power["idle"][1].get<double>();
	error_dividend = error_multiply(dividend, error_dividend,
	                                util["inter_random"]["runtime_avg"]);
	dividend *= util["inter_random"]["runtime_avg"][0].get<double>();
	dividend -= energy["spike"][0].get<double>() *
	            util["inter_random"]["number_of_spikes_avg"][0].get<double>();
	error_dividend += error_multiply(
	    energy["spike"], util["inter_random"]["number_of_spikes_avg"]);
	dividend -=
	    util["inter_random"]["number_of_spikes_tar_avg"][0].get<double>() *
	    energy["spike"][0].get<double>();
	error_dividend +=
	    util["inter_random"]["number_of_spikes_tar_avg"][1].get<double>() *
	    energy["spike"][1].get<double>();
	divisor = (util["inter_random"]["number_of_spikes_avg"][0].get<double>() *
	           util["inter_random"]["connections_avg"][0].get<double>());
	error_divisor = error_multiply(util["inter_random"]["number_of_spikes_avg"],
	                               util["inter_random"]["connections_avg"]);
	val = dividend / divisor;
	error = error_devide(dividend, error_dividend, divisor, error_divisor);
	energy["Transmission_random"] = {val, error};

	if (energy_model["stdp"].get<bool>()) {
		// Power of idle STDP synapse //
		dividend =
		    (measured["stdp_idle_avg"][0].get<double>() / 1000.0 -
		     util["stdp_idle"]["number_of_neurons_avg"][0].get<double>() *
		         power["idle_recorded_neurons"][0].get<double>() -
		     power["idle"][0].get<double>());
		error_dividend =
		    (measured["stdp_idle_avg"][1].get<double>() / 1000.0 +
		     error_multiply(util["stdp_idle"]["number_of_neurons_avg"],
		                    power["idle_recorded_neurons"]) +
		     power["idle"][1].get<double>());
		divisor = util["stdp_idle"]["number_of_neurons_avg"][0];
		error_divisor = util["stdp_idle"]["number_of_neurons_avg"][1];
		val = dividend / divisor;
		error = error_devide(dividend, error_dividend, divisor, error_divisor);
		power["idle_stdp"] = {val, error};

		// E per spike transmitted via STDP synapse
		dividend =
		    (measured["stdp_spike_avg"][0].get<double>() / 1000.0 -
		     power["idle"][0].get<double>() -
		     util["stdp_spike"]["number_of_neurons_avg"][0].get<double>() *
		         power["idle_stdp"][0].get<double>() -
		     util["stdp_spike"]["number_of_neurons_avg"][0].get<double>() *
		         power["idle_recorded_neurons"][0].get<double>());
		error_dividend =
		    (measured["stdp_spike_avg"][1].get<double>() / 1000.0 +
		     power["idle"][1].get<double>() +
		     error_multiply(util["stdp_spike"]["number_of_neurons_avg"],
		                    power["idle_stdp"]) +
		     error_multiply(util["stdp_spike"]["number_of_neurons_avg"],
		                    power["idle_recorded_neurons"]));
		error_dividend = error_multiply(dividend, error_dividend,
		                                util["stdp_spike"]["runtime_avg"]);
		dividend *= util["stdp_spike"]["runtime_avg"][0].get<double>();
		dividend -= energy["spike"][0].get<double>() *
		            util["stdp_spike"]["number_of_spikes_avg"][0].get<double>();
		error_dividend += error_multiply(
		    energy["spike"], util["stdp_spike"]["number_of_spikes_avg"]);
		double first =
		    util["stdp_spike"]["number_of_source_spikes_avg"][0].get<double>() *
		    energy["InputSpike_A2A"][0].get<double>();
		double error_first =
		    error_multiply(util["stdp_spike"]["number_of_source_spikes_avg"],
		                   energy["InputSpike_A2A"]);

		error_dividend += error_multiply(
		    first, error_first, util["stdp_spike"]["number_of_neurons_avg"]);
		dividend -=
		    first *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>();
		divisor =
		    util["stdp_spike"]["number_of_source_spikes_avg"][0].get<double>() *
		    util["stdp_spike"]["number_of_neurons_avg"][0].get<double>();
		error_divisor =
		    error_multiply(util["stdp_spike"]["number_of_source_spikes_avg"],
		                   util["stdp_spike"]["number_of_neurons_avg"]);
		val = dividend / divisor;
		error = error_devide(dividend, error_dividend, divisor, error_divisor);
		energy["Transmission_STDP"] = {val, error};
	}
}

std::pair<double, double> calculate_energy(const cypress::Network &netw,
                                           const Json &energy_model,
                                           double bioruntime)
{
	double runtime = netw.runtime().sim_pure * 1000.0;
	double energy = 0.0, error = 0.0;
	energy += runtime * energy_model["power"]["idle"][0].get<double>();
	error += runtime * energy_model["power"]["idle"][1].get<double>();
	auto &conns = netw.connections();
	bool runtime_normalized = false;
	if (energy_model.count("runtime_normalized") > 0 &&
	    energy_model["runtime_normalized"].get<bool>()) {
		runtime_normalized = true;
		if (bioruntime == 0) {
			bioruntime = netw.duration();
			cypress::global_logger().warn(
			    "EnergyModel", "Please provide simulation duration!");
		}
	}
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
			          energy_model["energy"]["InputSpike_O2O"][0].get<double>();
			energy += double(std::get<1>(spikes)) *
			          energy_model["energy"]["InputSpike_A2A"][0].get<double>();
			energy +=
			    double(std::get<2>(spikes)) *
			    energy_model["energy"]["InputSpike_random"][0].get<double>();

			error += double(std::get<0>(spikes)) *
			         energy_model["energy"]["InputSpike_O2O"][1].get<double>();
			error += double(std::get<1>(spikes)) *
			         energy_model["energy"]["InputSpike_A2A"][1].get<double>();
			error +=
			    double(std::get<2>(spikes)) *
			    energy_model["energy"]["InputSpike_random"][1].get<double>();
		}
		else {
			if (!pop.signals().is_recording(0)) {
				if (!runtime_normalized) {
					energy +=
					    double(pop.size()) *
					    energy_model["power"]["idle_neurons"][0].get<double>() *
					    runtime;
					error +=
					    double(pop.size()) *
					    energy_model["power"]["idle_neurons"][1].get<double>() *
					    runtime;
					cypress::global_logger().warn(
					    "EnergyModel",
					    "Please activate spike recording for all populations!");
					continue;
				}
				else {
					energy += double(pop.size()) *
					          energy_model["energy"]["idle_neurons_ms"][0]
					              .get<double>() *
					          bioruntime;
					error += double(pop.size()) *
					         energy_model["energy"]["idle_neurons_ms"][1]
					             .get<double>() *
					         bioruntime;
					cypress::global_logger().warn(
					    "EnergyModel",
					    "Please activate spike recording for all populations!");
					continue;
				}
			}
			if (!runtime_normalized) {
				energy += double(pop.size()) *
				          energy_model["power"]["idle_recorded_neurons"][0]
				              .get<double>() *
				          bioruntime;
				error += double(pop.size()) *
				         energy_model["power"]["idle_recorded_neurons"][1]
				             .get<double>() *
				         bioruntime;
			}
			else {
				energy += double(pop.size()) *
				          energy_model["energy"]["idle_recorded_neurons_ms"][0]
				              .get<double>() *
				          bioruntime;
				error += double(pop.size()) *
				         energy_model["energy"]["idle_recorded_neurons_ms"][1]
				             .get<double>() *
				         bioruntime;
			}

			auto spikes = get_number_of_spikes_pop(pop);
			energy += double(spikes) *
			          energy_model["energy"]["spike"][0].get<double>();
			error += double(spikes) *
			         energy_model["energy"]["spike"][1].get<double>();
			auto spikes2 = calc_postsyn_spikes(pop, conns, false);
			energy +=
			    double(std::get<0>(spikes2)) *
			    energy_model["energy"]["Transmission_O2O"][0].get<double>();
			energy +=
			    double(std::get<1>(spikes2)) *
			    energy_model["energy"]["Transmission_S2A"][0].get<double>();
			energy +=
			    double(std::get<2>(spikes2)) *
			    energy_model["energy"]["Transmission_random"][0].get<double>();
			error +=
			    double(std::get<0>(spikes2)) *
			    energy_model["energy"]["Transmission_O2O"][1].get<double>();
			error +=
			    double(std::get<1>(spikes2)) *
			    energy_model["energy"]["Transmission_S2A"][1].get<double>();
			error +=
			    double(std::get<2>(spikes2)) *
			    energy_model["energy"]["Transmission_random"][1].get<double>();
		}
	}

	if (energy_model["stdp"].get<bool>()) {

		if (!runtime_normalized) {
			energy += double(calc_number_stdp_synapses(netw)) *
			          energy_model["power"]["idle_stdp"][0].get<double>();
			error += double(calc_number_stdp_synapses(netw)) *
			         energy_model["power"]["idle_stdp"][1].get<double>();
		}
		else {
			energy += double(calc_number_stdp_synapses(netw)) *
			          energy_model["energy"]["idle_stdp_ms"][0].get<double>();
			error += double(calc_number_stdp_synapses(netw)) *
			         energy_model["energy"]["idle_stdp_ms"][1].get<double>();
		}
		for (const auto &pop : netw.populations()) {
			if (pop.signals().is_recording(0)) {
				auto spikes2 = calc_postsyn_spikes(pop, conns, true);
				energy += double(std::get<0>(spikes2)) *
				          energy_model["energy"]["Transmission_STDP"][0]
				              .get<double>();
				energy += double(std::get<1>(spikes2)) *
				          energy_model["energy"]["Transmission_STDP"][0]
				              .get<double>();
				energy += double(std::get<2>(spikes2)) *
				          energy_model["energy"]["Transmission_STDP"][0]
				              .get<double>();
				error += double(std::get<0>(spikes2)) *
				         energy_model["energy"]["Transmission_STDP"][1]
				             .get<double>();
				error += double(std::get<1>(spikes2)) *
				         energy_model["energy"]["Transmission_STDP"][1]
				             .get<double>();
				error += double(std::get<2>(spikes2)) *
				         energy_model["energy"]["Transmission_STDP"][1]
				             .get<double>();
			}
		}
	}
	return std::pair<double, double>{energy, error};
}

}  // namespace Energy
