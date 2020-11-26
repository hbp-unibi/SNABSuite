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
//#define EIGEN_DEFAULT_DENSE_INDEX_TYPE size_t
#include <Eigen/Dense>
#include <cypress/backend/power/power.hpp>  // Control of power via netw
#include <cypress/cypress.hpp>              // Neural network frontend
#include <cypress/nef.hpp>

#include "function.hpp"
#include "mnist/mnist_mlp.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;
FunctionApproximation::FunctionApproximation(const std::string backend,
                                             size_t bench_index)
    : SNABBase(__func__, backend,
               {"Average approximation error", "Total approximation error"},
               {"quality", "quality"}, {"", ""}, {"", ""},
               {"neuron_type", "neuron_params", "#neurons", "#repeat",
                "#samples_test", "#repeat_test", "weight", "bias_weight",
                "bias_weight_inh", "response_time", "min_spike_interval"},
               bench_index)
{
}

Network &FunctionApproximation::build_netw(Network &netw)
{

	const size_t n_samples =
	    m_config_file["#neurons"]
	        .get<size_t>();  // Sample points on the tuning curve
	const size_t n_repeat =
	    m_config_file["#repeat"].get<size_t>();  // Repetitions for each sample
	const size_t n_samples_test =
	    m_config_file["#samples_test"]
	        .get<size_t>();  // Sample points on the tuning curve
	const size_t n_repeat_test =
	    m_config_file["#repeat_test"]
	        .get<size_t>();  // Repetitions for each sample
	const size_t n_neurons =
	    m_config_file["#neurons"].get<size_t>();  // Number of neurons
	const Real exc_synaptic_weight = m_config_file["weight"].get<Real>();  // uS
	const Real exc_bias_synaptic_weight =
	    m_config_file["bias_weight"].get<Real>();  // uS
	const Real inh_synaptic_weight =
	    m_config_file["bias_weight_inh"].get<Real>();  // uS
	const Real response_time =
	    m_config_file["response_time"].get<Real>();  // ms
	const Real min_spike_interval =
	    m_config_file["min_spike_interval"].get<Real>();  // ms
	RNG::instance().seed(1234);

	std::string neuron_type_str = m_config_file["neuron_type"];
	// Get neuron neuron_parameters
	m_neuro_params = NeuronParameter(SpikingUtils::detect_type(neuron_type_str),
	                                 m_config_file["neuron_params"]);

	// const Real min_spike_interval = m_neuro_params.get("tau_refrac");  // ms

	// Setup the tuning curve generator/evaluator
	m_evaluator_train = nef::TuningCurveEvaluator(
	    n_samples, n_repeat, min_spike_interval * 1e-3, response_time * 1e-3);
	m_evaluator_test = nef::TuningCurveEvaluator(n_samples_test, n_repeat_test,
	                                             min_spike_interval * 1e-3,
	                                             response_time * 1e-3);
	const Real larger_len = std::max(m_evaluator_train.input_spike_train_len(),
	                                 m_evaluator_test.input_spike_train_len());

	// Source Population
	Population<SpikeSourceArray> pop_src(
	    netw, 1, m_evaluator_train.input_spike_train(), "input");

	// Bias source population
	std::vector<Real> bias_spikes =
	    spikes::constant_interval(0.0, larger_len, min_spike_interval);
	Population<SpikeSourceArray> pop_src_bias(netw, 1, bias_spikes, "bias");

	// Target population
	auto pop_tar = SpikingUtils::add_population(
	    neuron_type_str, netw, m_neuro_params, n_neurons, "spikes");
	pop_tar.name("target");

	// Connect the source population to the target neurons, choose inhibitory
	// connections for every second neuron
	pop_src.connect_to(pop_tar, Connector::functor(
	                                [](NeuronIndex, NeuronIndex tar) {
		                                return (tar % 2) == 0;
	                                },
	                                exc_synaptic_weight));
	pop_src.connect_to(pop_tar, Connector::functor(
	                                [](NeuronIndex, NeuronIndex tar) {
		                                return (tar % 2) == 1;
	                                },
	                                inh_synaptic_weight));

	// Connect the bias spike source to every second target neuron
	pop_src_bias.connect_to(pop_tar, Connector::functor(
	                                     [](NeuronIndex, NeuronIndex tar) {
		                                     return (tar % 2) == 1;
	                                     },
	                                     exc_bias_synaptic_weight));

	// Diversification of neurons
	if (m_config_file.find("random_bias") != m_config_file.end()) {
		Real rate = m_config_file["random_bias"]["rate"].get<Real>();
		Real std_dev = m_config_file["random_bias"]["std_dev"].get<Real>();
		Real weight = m_config_file["random_bias"]["weight"].get<Real>();
		Real weight_inh =
		    m_config_file["random_bias"]["weight_inh"].get<Real>();
		std::vector<LocalConnection> conns, conns_inh;
		auto &rng = RNG::instance().get();
		std::normal_distribution<Real> distribution(weight, std_dev);
		std::normal_distribution<Real> distribution_inh(weight_inh, std_dev);

		for (size_t i = 0; i < pop_tar.size(); i++) {
			conns.emplace_back(
			    LocalConnection(0, i, std::max(distribution(rng), 0.0), 1.0));
			conns_inh.emplace_back(LocalConnection(
			    0, i, std::min(distribution_inh(rng), 0.0), 1.0));
		}
		std::vector<Real> bias_spikes2 =
		    spikes::constant_frequency(0.0, larger_len, rate);
		Population<SpikeSourceArray> pop_src_bias2(netw, 1, bias_spikes2,
		                                           "bias_weight");
		pop_src_bias2.connect_to(pop_tar, Connector::from_list(conns));
		pop_src_bias2.connect_to(pop_tar, Connector::from_list(conns_inh));
	}

	if (m_config_file.find("random_bias_spikes") != m_config_file.end()) {
		Real rate = m_config_file["random_bias_spikes"]["rate"].get<Real>();
		Real std_dev =
		    m_config_file["random_bias_spikes"]["std_dev"].get<Real>();
		Real weight = m_config_file["random_bias_spikes"]["weight"].get<Real>();
		Real weight_inh =
		    m_config_file["random_bias_spikes"]["weight_inh"].get<Real>();
		auto &rng = RNG::instance().get();
		std::normal_distribution<Real> distribution(rate, std_dev);

		Population<SpikeSourceConstFreq> pop_src_bias2(
		    netw, pop_tar.size(),
		    SpikeSourceConstFreqParameters().start(0).duration(larger_len),
		    "bias_spike");
		Population<SpikeSourceConstFreq> pop_src_bias2_inh(
		    netw, pop_tar.size(),
		    SpikeSourceConstFreqParameters().start(0).duration(larger_len),
		    "bias_spike_inh");
		for (size_t i = 0; i < pop_tar.size(); i++) {
			pop_src_bias2[i].parameters().rate(
			    std::max(distribution(rng), 0.0));
			pop_src_bias2_inh[i].parameters().rate(
			    std::max(distribution(rng), 0.0));
		}
		pop_src_bias2.connect_to(pop_tar, Connector::one_to_one(weight, 1.0));
		pop_src_bias2_inh.connect_to(pop_tar,
		                             Connector::one_to_one(weight_inh, 1.0));
	}

	if (m_config_file.find("random_thresh") != m_config_file.end()) {
		auto ind = pop_tar.type().parameter_index("v_thresh");
		if (!ind.valid()) {
			throw std::invalid_argument(
			    "Could not resolve index for v_thresh!");
		}
		Real v_thresh = pop_tar.parameters().parameters()[ind.value()];
		Real std_dev = m_config_file["random_thresh"]["std_dev"].get<Real>();

		auto &rng = RNG::instance().get();
		std::normal_distribution<Real> distribution(v_thresh, std_dev);
		for (auto neuron : pop_tar) {
			neuron.parameters().set(ind.value(),
			                        std::max(v_thresh + 2, distribution(rng)));
		}
	}
	return netw;
}

void FunctionApproximation::run_netw(Network &netw)
{
	// Debug logger, may be ignored in the future
	netw.logger().min_level(ERROR, 0);
	m_netw_test = netw.clone();

	// Run the calibration network
	PowerManagementBackend pwbackend(Network::make_backend(m_backend));
	netw.run(pwbackend, m_evaluator_train.input_spike_train_len());

	// Run the test network
	m_netw_test.population<SpikeSourceArray>("input").parameters().spike_times(
	    m_evaluator_test.input_spike_train());

	m_netw_test.run(pwbackend, m_evaluator_test.input_spike_train_len());

	m_netw_train = netw;
}

namespace {
using EMatrix = Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>;
using EVector = Eigen::Matrix<Real, Eigen::Dynamic, 1>;
/**
 * @brief Returns the response of the population encoded in values [0,1]
 *
 * @param pop_tar the target population
 * @param eval The TuningCurveEvaluator used for generating the input spike
 * train
 * @param n_samples number of samples (not repetitions)
 * @return A pair of data structures, encoding the x values (first part) and the
 * y values/responses of the neurons; mat(i,j) is the response of the j-th
 * neuron to input vec[i].
 */
std::pair<std::vector<Real>, EMatrix> get_responses(
    PopulationBase &pop_tar, nef::TuningCurveEvaluator &eval, size_t n_samples)
{
	size_t n_neurons = pop_tar.size();
	std::pair<std::vector<Real>, EMatrix> ret(std::vector<Real>(n_samples, 0.0),
	                                          EMatrix(n_samples, n_neurons));
	for (auto neuron : pop_tar) {
		// Calculate the tuning curve for this neuron in the population
		std::vector<std::pair<Real, Real>> res =
		    eval.evaluate_output_spike_train(neuron.signals().data(0));

		// Write the results to the result matrix
		for (size_t i = 0; i < res.size(); i++) {
			ret.second(i, neuron.nid()) = res[i].second;
		}
		if (neuron.nid() == 0) {
			for (size_t i = 0; i < res.size(); i++) {
				ret.first[i] = res[i].first;
			}
		}
	}

	return ret;
}

/**
 * @brief Calculates the function value given by the encoding of coeff. The row
 * defines for which input the function value should be computed.
 *
 * @param coeff coefficients to be multiplied with the response of the neurons
 * @param response Response matrix
 * @param row calculates network answer for x[row]
 * @return cypress::Real the function value approximated by the network
 */
Real inline get_function_value_from_result(const EVector &coeff,
                                           const EMatrix &response,
                                           const size_t &row)
{
	Real res = 0.0;
	for (ptrdiff_t i = 0; i < response.cols(); i++) {
		res += coeff[i] * response(row, i);
	}
	return res;
}

#if SNAB_DEBUG
/**
 * @brief Plot relevant spikes of a tuning curve network
 *
 * @param netw the network
 * @param filename where to store spikes
 * @param backend the name of the backend
 */
void plot_spikes(const NetworkBase &netw, std::string filename,
                 std::string backend)
{
	std::vector<std::vector<cypress::Real>> spikes;
	spikes.emplace_back(netw.populations("input")[0].parameters().parameters());
	spikes.emplace_back(netw.populations("bias")[0].parameters().parameters());
	auto pop_tar = netw.populations("target")[0];

	for (const auto &i : pop_tar) {
		spikes.push_back(i.signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes, filename);

	// Trigger plots
	Utilities::plot_spikes(filename, backend);
}

/**
 * @brief Pretty print the response of the network
 *
 * @param response result of get_responses
 */
void print_response(const std::pair<std::vector<Real>, const EMatrix> &response)
{
	auto &x = response.first;
	auto &ys = response.second;
	std::cout << "x\tys\n";
	for (ptrdiff_t i = 0; i < ys.rows(); i++) {
		std::cout << x[i] << "\t";
		for (ptrdiff_t j = 0; j < ys.cols() - 1; j++) {
			std::cout << ys(i, j) << ", ";
		}
		std::cout << ys(i, ys.cols() - 1) << std::endl;
	}
	std::cout << std::endl;
}

void plot_response(const std::pair<std::vector<Real>, const EMatrix> &response,
                   std::string filename)
{
	auto &x = response.first;
	auto &ys = response.second;
	pyplot::figure_size(600, 400);
	pyplot::title("Activation Curves");
	for (ptrdiff_t j = 0; j < ys.cols(); j++) {
		std::vector<Real> y;
		for (ptrdiff_t i = 0; i < ys.rows(); i++) {
			y.emplace_back(ys(i, j));
		}
		std::map<std::string, std::string> keywords;
		if (j % 2) {
			keywords["color"] = "blue";
		}
		else {
			keywords["color"] = "red";
		}
		pyplot::plot(x, y, keywords);
	}
	pyplot::tight_layout();
	pyplot::save(filename);
}
#endif

/**
 * @brief Calculates the target values and the network approximated values of
 * the network.
 *
 * @param Func the function to be approximated (template)
 * @param f the function to be approximated
 * @param inverse The inverse matrix of the pre_train response
 * @param pre_train_x x values for the pre_train run
 * @param post_train response of the second network
 * @return pairs of values (target, approximated)
 */
template <typename Func>
std::vector<std::pair<Real, Real>> evaluate_for_function(
    const Func &f, const std::pair<std::vector<Real>, const EMatrix> &pre_train,
    const std::pair<std::vector<Real>, const EMatrix> &post_train)
{

	// Calculate the coefficients of the approximation
	EVector function_values(pre_train.first.size());
	for (size_t i = 0; i < pre_train.first.size(); i++) {
		function_values[i] = f(pre_train.first[i]);
	}
	EVector coeff =
	    pre_train.second.colPivHouseholderQr().solve(function_values);

	// Calculate target value and network response value
	std::vector<std::pair<Real, Real>> res;
	for (size_t i = 0; i < post_train.first.size(); i++) {
		Real target = f(post_train.first[i]);
		Real approx =
		    get_function_value_from_result(coeff, post_train.second, i);
		res.emplace_back(std::pair<Real, Real>(target, approx));
	}
	return res;
}

std::vector<std::array<Real, 4>> calculate_statistics(
    std::vector<std::pair<Real, Real>> &values)
{
	std::vector<Real> deviations;
	for (auto &i : values) {
		deviations.emplace_back(i.first - i.second);
	}
	// Calculate statistics
	Real max, min, avg, std_dev;
	Utilities::calculate_statistics(deviations, min, max, avg, std_dev);

	Real total = 0.0;
	for (auto &i : deviations) {
		total += fabs(i);
	}

	return {std::array<Real, 4>({avg, std_dev, min, max}),
	        std::array<Real, 4>({total, NaN(), NaN(), NaN()})};
}

#if SNAB_DEBUG
void plot_function(std::vector<Real> x,
                   std::vector<std::pair<Real, Real>> &values,
                   std::string filename)
{
	std::vector<Real> target, approx;
	for (auto &i : values) {
		target.emplace_back(i.first);
		approx.emplace_back(i.second);
	}

	pyplot::figure_size(600, 400);
	pyplot::title("Spiking Function Approximation");
	pyplot::named_plot("Target Function", x, target);
	pyplot::named_plot("Approximation", x, approx);

	pyplot::legend();
	pyplot::tight_layout();
	pyplot::save(filename);
}
#endif
}  // namespace

std::vector<std::array<Real, 4>> FunctionApproximation::evaluate()
{

	// Read out responses of both networks
	auto pop_tar = m_netw_train.populations("target")[0];
	const size_t n_samples = m_config_file["#neurons"].get<size_t>();
	const size_t n_samples_test = m_config_file["#samples_test"].get<size_t>();
	auto pre_train = get_responses(pop_tar, m_evaluator_train, n_samples);
	pop_tar = m_netw_test.populations("target")[0];
	auto post_train = get_responses(pop_tar, m_evaluator_test, n_samples_test);

#if SNAB_DEBUG
	plot_spikes(m_netw_train, _debug_filename("spikes_train.csv"), m_backend);
	plot_spikes(m_netw_test, _debug_filename("spikes_test.csv"), m_backend);

	std::cout << "Pre Train\n";
	print_response(pre_train);
	plot_response(pre_train, _debug_filename("activation_curve_train.png"));
	std::cout << "Post Train\n";
	print_response(post_train);
	plot_response(post_train, _debug_filename("activation_curve_test.png"));
#endif

	// Calculate the inverse matrix if possible
	/*Matrix<Real> inverse;
	try {
	    inverse = pre_train.second.inverse();
	}
	catch (std::invalid_argument &er) {
	    global_logger().error(
	        "SNABSuite",
	        "Could not calculate inverse matrix: " + std::string(er.what()));
	    return {std::array<cypress::Real, 4>({0, 0, 0, 0}),
	            std::array<cypress::Real, 4>({0, 0, 0, 0})};
	}
	// Define the function to be approximated*/
	auto function = [](Real x) { return x; };
	auto res = evaluate_for_function(function, pre_train, post_train);
#if SNAB_DEBUG
	plot_function(post_train.first, res, _debug_filename("function.png"));
#endif
	return calculate_statistics(res);
}
}  // namespace SNAB
