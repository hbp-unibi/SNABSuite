/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2019  Christoph Ostrau
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
#include <cypress/cypress.hpp>  // Neural network frontend
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "mnist.hpp"
#include "mnist_mlp.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;
MNIST_BASE::MNIST_BASE(const std::string backend, size_t bench_index)
    : MNIST_BASE(backend, bench_index, __func__)
{
}

MNIST_BASE::MNIST_BASE(const std::string backend, size_t bench_index,
                       std::string name)
    : SNABBase(name, backend, {"accuracy", "sim_time"},
               {"quality", "performance"}, {"accuracy", "time"}, {"", "s"},
               {"neuron_type", "neuron_params", "images", "batchsize",
                "duration", "max_freq", "pause", "poisson", "max_weight",
                "train_data", "batch_parallel", "dnn_file", "scaled_image"},
               bench_index)
{
}

void MNIST_BASE::read_config()
{
	m_neuron_type_str = m_config_file["neuron_type"].get<std::string>();

	m_neuro_params =
	    NeuronParameter(SpikingUtils::detect_type(m_neuron_type_str),
	                     m_config_file["neuron_params"]);
	if (m_neuron_type_str == "IF_cond_exp") {
		m_neuro_params.set("tau_syn_I", m_neuro_params.get("tau_syn_E"));
	}
	m_images = m_config_file["images"].get<size_t>();
	m_batchsize = m_config_file["batchsize"].get<size_t>();
	m_duration = m_config_file["duration"].get<Real>();
	m_max_freq = m_config_file["max_freq"].get<Real>();
	m_pause = m_config_file["pause"].get<Real>();
	m_poisson = m_config_file["poisson"].get<bool>();
	m_max_weight = m_config_file["max_weight"].get<Real>();
	m_train_data = m_config_file["train_data"].get<bool>();
	m_batch_parallel = m_config_file["batch_parallel"].get<bool>();
	m_dnn_file = m_config_file["dnn_file"].get<std::string>();
	m_scaled_image = m_config_file["scaled_image"].get<bool>();
	m_weights_scale_factor = 0.0;
	if (m_config_file.find("count_spikes") != m_config_file.end()) {
		m_count_spikes = m_config_file["count_spikes"].get<bool>();
	}
}

cypress::Network &MNIST_BASE::build_netw_int(cypress::Network &netw)
{
	read_config();
	auto kerasdata = mnist_helper::read_network(m_dnn_file, true);
	m_mlp = std::make_shared<MNIST::MLP<>>(kerasdata, 0, m_batchsize, 0.0);
	if (m_scaled_image) {
		m_mlp->scale_down_images();
	}
	mnist_helper::SPIKING_MNIST spike_mnist;
	if (m_train_data) {
		spike_mnist =
		    mnist_helper::mnist_to_spike(m_mlp->mnist_train_set(), m_duration,
		                                 m_max_freq, m_images, m_poisson);
	}
	else {
		spike_mnist =
		    mnist_helper::mnist_to_spike(m_mlp->mnist_test_set(), m_duration,
		                                 m_max_freq, m_images, m_poisson);
	}
	m_batch_data = mnist_helper::create_batches(spike_mnist, m_batchsize,
	                                            m_duration, m_pause, false);

	m_label_pops.clear();
	if (m_batch_parallel) {
		m_networks.clear();
		m_all_pops.clear();
		for (auto &i : m_batch_data) {
			mnist_helper::create_spike_source(netw, i);
			create_deep_network(netw, m_max_weight);
			m_label_pops.emplace_back(netw.populations().back());
		}

		if (m_count_spikes) {
			for (auto pop : netw.populations()) {
				pop.signals().record(0);
				m_all_pops.emplace_back(pop);
			}
		}
	}
	else {
		m_networks.clear();
		for (auto &i : m_batch_data) {
			m_networks.push_back(cypress::Network());
			mnist_helper::create_spike_source(m_networks.back(), i);
			create_deep_network(m_networks.back(), m_max_weight);
			m_label_pops.emplace_back(m_networks.back().populations().back());
			if (m_count_spikes) {
				for (auto pop : m_networks.back().populations()) {
					pop.signals().record(0);
					m_all_pops.emplace_back(pop);
				}
			}
		}
	}

	for (auto &pop : m_label_pops) {
		pop.signals().record(0);
	}

#if SNAB_DEBUG
	Utilities::write_vector2_to_csv(std::get<0>(m_batch_data[0]),
	                                _debug_filename("spikes_input.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_input.csv"), m_backend);
#endif
	return netw;
}

cypress::Network &MNIST_BASE::build_netw(cypress::Network &netw)
{
	return build_netw_int(netw);
}

void MNIST_BASE::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	try {

		if (m_batch_parallel) {
			netw.run(pwbackend, m_batchsize * (m_duration + m_pause));
		}
		else {
			global_logger().info(
			    "SNABSuite",
			    "batch not run in parallel, using internal network objects!");
			for (auto &pop : m_label_pops) {
				pop.network().run(pwbackend,
				                  m_batchsize * (m_duration + m_pause));
			}
		}
	}
	catch (const std::exception &exc) {
		std::cerr << exc.what();
		global_logger().fatal_error(
		    "SNABSuite",
		    "Wrong parameter setting or backend error! Simulation broke down");
	}
}

std::vector<std::array<cypress::Real, 4>> MNIST_BASE::evaluate()
{
	size_t global_correct(0);
	size_t images(0);
	for (size_t batch = 0; batch < m_label_pops.size(); batch++) {
		auto pop = m_label_pops[batch];
		auto labels = mnist_helper::spikes_to_labels(pop, m_duration, m_pause,
		                                             m_batchsize);
		auto &orig_labels = std::get<1>(m_batch_data[batch]);
		auto correct = mnist_helper::compare_labels(orig_labels, labels);
		global_correct += correct;
		images += orig_labels.size();

#if SNAB_DEBUG
		std::cout << "Target\t Infer" << std::endl;
		for (size_t i = 0; i < orig_labels.size(); i++) {
			std::cout << orig_labels[i] << "\t" << labels[i] << std::endl;
		}
		std::vector<std::vector<cypress::Real>> spikes;
		for (size_t i = 0; i < pop.size(); i++) {
			spikes.push_back(pop[i].signals().data(0));
		}
		Utilities::write_vector2_to_csv(
		    spikes,
		    _debug_filename("spikes_" + std::to_string(batch) + ".csv"));
		Utilities::plot_spikes(
		    _debug_filename("spikes_" + std::to_string(batch) + ".csv"),
		    m_backend);
#endif
	}
	if (m_count_spikes) {
		size_t global_count = 0;
		for (auto &pop : m_all_pops) {
			size_t count = 0.0;
			for (auto neuron : pop) {
				count += neuron.signals().data(0).size();
			}
			global_count += count;
			global_logger().info(
			    "SNABSuite", "Pop " + std::to_string(pop.pid()) +
			                     " with size " + std::to_string(pop.size()) +
			                     " fired " + std::to_string(count) + " spikes");
		}
		global_logger().info(
		    "SNABSuite",
		    "Summ of all spikes: " + std::to_string(global_count) + " spikes");
	}
	Real acc = Real(global_correct) / Real(images);
	Real sim_time = m_netw.runtime().sim;
	if (!m_batch_parallel) {
		sim_time = 0.0;
		for (auto &pop : m_label_pops) {
			sim_time += pop.network().runtime().sim;
		}
	}
	return {std::array<cypress::Real, 4>({acc, NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({sim_time, NaN(), NaN(), NaN()})};
}

size_t MNIST_BASE::create_deep_network(Network &netw, Real max_weight)
{
	size_t layer_id = netw.populations().size();
	// size_t counter = 0;
	if (m_weights_scale_factor == 0.0) {
		if (max_weight > 0) {
			m_weights_scale_factor = max_weight / m_mlp->max_weight();
		}
		else {
			m_weights_scale_factor = 1.0;
		}
	}

	size_t dense_counter = 0;
	size_t conv_counter = 0;
	size_t pool_counter = 0;
	for (const auto &layer : m_mlp->get_layer_types()){
        if (layer == mnist_helper::Dense){
			const auto &layer_weights = m_mlp->get_weights()[dense_counter];
            size_t size = layer_weights.cols();
            auto pop = SpikingUtils::add_population(m_neuron_type_str, netw,
                                                    m_neuro_params, size, "");
            auto conns = mnist_helper::dense_weights_to_conn(
                layer_weights, m_weights_scale_factor, 1.0);
            netw.add_connection(netw.populations()[layer_id - 1], pop,
                                Connector::from_list(conns),
                                ("dense_" + std::to_string(dense_counter)).c_str());

            global_logger().debug(
                "SNABSuite",
                "Dense layer constructed with size " + std::to_string(size));

			dense_counter++;
		} else if (layer == mnist_helper::Conv){
			const auto &layer_weights = m_mlp->get_filter_weights()[conv_counter];
            size_t size = layer_weights.output_sizes[0] * layer_weights.output_sizes[1] * layer_weights.output_sizes[2];
			auto pop = SpikingUtils::add_population(m_neuron_type_str, netw,
			                                        m_neuro_params, size, "");
			auto conns = mnist_helper::conv_weights_to_conn(
			    layer_weights, m_weights_scale_factor, 1.0);
			netw.add_connection(netw.populations()[layer_id - 1], pop,
			                    Connector::from_list(conns),
                                ("conv_" + std::to_string(conv_counter)).c_str());
            global_logger().debug(
			    "SNABSuite",
			    "Convolution layer constructed with size " + std::to_string(size));
			conv_counter++;
		} else if (layer == mnist_helper::Pooling){
			auto &pool_layer = m_mlp->get_pooling_layers()[pool_counter];
			size_t size = pool_layer.output_sizes[0] * pool_layer.output_sizes[1]
			                * pool_layer.output_sizes[2];
			auto pop = SpikingUtils::add_population(m_neuron_type_str, netw,
			                                        m_neuro_params, size, "");
			auto conns = mnist_helper::pool_to_conn(pool_layer, 1.0);
			netw.add_connection(netw.populations()[layer_id - 1], netw.populations()[layer_id - 1],
			                    Connector::from_list(conns[0]), "dummy_name");
			netw.add_connection(netw.populations()[layer_id - 1], pop,
			                    Connector::from_list(conns[1]),
                                ("pool_" + std::to_string(pool_counter)).c_str());
			global_logger().debug(
			    "SNABSuite",
			    "Pooling layer constructed with size " + std::to_string(size) +
			    " and " + std::to_string(conns[0].size()) + " inhibitory connections");
			pool_counter++;
		}
        layer_id++;
	}
	return dense_counter+conv_counter+pool_counter;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

cypress::Network &MnistITLLastLayer::build_netw(cypress::Network &netw)
{
	read_config();

	if (m_config_file.find("positive") != m_config_file.end()) {
		m_positive = m_config_file["positive"].get<bool>();
	}
	if (m_config_file.find("norm_rate_hidden") != m_config_file.end()) {
		m_norm_rate_hidden = m_config_file["norm_rate_hidden"].get<Real>();
	}
	if (m_config_file.find("norm_rate_last") != m_config_file.end()) {
		m_norm_rate_last = m_config_file["norm_rate_last"].get<Real>();
	}
	if (m_config_file.find("loss_function") != m_config_file.end()) {
		m_loss_function = m_config_file["loss_function"].get<std::string>();
	}
	bool random_init = false;
	if (m_config_file.find("random_init") != m_config_file.end()) {
		random_init = m_config_file["random_init"].get<bool>();
	}
	if (m_config_file.find("num_test_images") != m_config_file.end()) {
		m_num_test_images = m_config_file["num_test_images"].get<size_t>();
	}
	if (m_config_file.find("test_batchsize") != m_config_file.end()) {
		m_test_batchsize = m_config_file["test_batchsize"].get<size_t>();
	}
	else {
		m_test_batchsize = m_batchsize;
	}

	// TODO ? Required -> constructor

	auto kerasdata = mnist_helper::read_network(m_dnn_file, true);
	if (m_positive) {
		if (m_loss_function == "CatHinge")
			m_mlp = std::make_shared<MNIST::MLP<MNIST::CatHinge, MNIST::ReLU,
			                                    MNIST::PositiveLimitedWeights>>(
			    kerasdata, m_config_file["epochs"].get<size_t>(), m_batchsize,
			    m_config_file["learn_rate"].get<Real>(), random_init);
		else if (m_loss_function == "MSE")
			m_mlp = std::make_shared<MNIST::MLP<MNIST::MSE, MNIST::ReLU,
			                                    MNIST::PositiveLimitedWeights>>(
			    kerasdata, m_config_file["epochs"].get<size_t>(), m_batchsize,
			    m_config_file["learn_rate"].get<Real>(), random_init);
		else {
			throw std::runtime_error("Unknown loss function " +
			                         m_loss_function);
		}
	}
	else {
		if (m_loss_function == "CatHinge")
			m_mlp = std::make_shared<
			    MNIST::MLP<MNIST::CatHinge, MNIST::ReLU, MNIST::NoConstraint>>(
			    kerasdata, m_config_file["epochs"].get<size_t>(), m_batchsize,
			    m_config_file["learn_rate"].get<Real>(), random_init);
		else if (m_loss_function == "MSE")
			m_mlp = std::make_shared<
			    MNIST::MLP<MNIST::MSE, MNIST::ReLU, MNIST::NoConstraint>>(
			    kerasdata, m_config_file["epochs"].get<size_t>(), m_batchsize,
			    m_config_file["learn_rate"].get<Real>(), random_init);
		else {
			throw std::runtime_error("Unknown loss function " +
			                         m_loss_function);
		}
	}

	if (m_scaled_image) {
		m_mlp->scale_down_images();
	}
	m_spmnist = mnist_helper::mnist_to_spike(
	    m_mlp->mnist_train_set(), m_duration, m_max_freq, m_images, m_poisson);
	return netw;
}

void MnistITLLastLayer::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));

	auto source_n = netw.create_population<SpikeSourceArray>(
	    m_mlp->get_layer_sizes()[0], SpikeSourceArrayParameters(),
	    SpikeSourceArraySignals(), "input_layer");

	create_deep_network(netw, m_max_weight);
	m_label_pops = {netw.populations().back()};

	auto pre_last_pop = netw.populations()[netw.populations().size() - 2];
	if (m_last_layer_only && !m_count_spikes) {
		m_label_pops[0].signals().record(0);
		pre_last_pop.signals().record(0);
	}
	else {
		m_all_pops.clear();
		for (auto pop : netw.populations()) {
			pop.signals().record(0);
			m_all_pops.emplace_back(pop);
		}
	}

	std::vector<std::vector<Real>> accuracies;
	size_t counter = 0;
	for (size_t train_run = 0; train_run < m_config_file["epochs"];
	     train_run++) {
		m_batch_data = mnist_helper::create_batches(m_spmnist, m_batchsize,
		                                            m_duration, m_pause, true);
		for (auto &i : m_batch_data) {
			if (std::get<1>(i).size() != m_batchsize) {
				continue;
			}
			mnist_helper::update_spike_source(source_n, i);
			netw.run(pwbackend, m_batchsize * (m_duration + m_pause));

			std::vector<std::vector<std::vector<Real>>> output_rates;
			for (auto &pop : netw.populations()) {
				if (pop.signals().is_recording(0)) {
					if (pop.pid() != netw.populations().back().pid()) {
						output_rates.emplace_back(mnist_helper::spikes_to_rates(
						    pop, m_duration, m_pause, m_batchsize,
						    m_norm_rate_hidden));
					}
					else {
						output_rates.emplace_back(mnist_helper::spikes_to_rates(
						    pop, m_duration, m_pause, m_batchsize,
						    m_norm_rate_last));
					}
				}
				else {
					output_rates.emplace_back(std::vector<std::vector<Real>>());
				}
			}
			m_mlp->backward_path_2(std::get<1>(i), output_rates,
			                       m_last_layer_only);

			mnist_helper::update_conns_from_mat(m_mlp->get_weights(), netw, 1.0,
			                                    m_weights_scale_factor);

			// Calculate batch accuracy
			auto labels = mnist_helper::spikes_to_labels(
			    m_label_pops[0], m_duration, m_pause, m_batchsize);
			m_global_correct =
			    mnist_helper::compare_labels(std::get<1>(i), labels);
			m_num_images = std::get<1>(i).size();
			m_sim_time = netw.runtime().sim;
			global_logger().debug(
			    "SNABsuite",
			    "Batch accuracy: " + std::to_string(Real(m_global_correct) /
			                                        Real(m_num_images)));

			accuracies.emplace_back(
			    std::vector<Real>{Real(counter) / Real(m_batch_data.size()),
			                      Real(m_global_correct) / Real(m_num_images)});
			counter++;
		}
	}

	if (m_train_data == false) {
		m_global_correct = 0;
		m_num_images = 0;
		m_sim_time = 0.0;
		size_t global_count = 0;
		std::vector<size_t> local_count, pop_size, pids;
		auto test_data = mnist_helper::mnist_to_spike(
		    m_mlp->mnist_test_set(), m_duration, m_max_freq, m_num_test_images,
		    m_poisson);
		m_batch_data = mnist_helper::create_batches(test_data, m_test_batchsize,
		                                            m_duration, m_pause, true);
		for (auto &i : m_batch_data) {
			mnist_helper::update_spike_source(source_n, i);
			netw.run(pwbackend, m_test_batchsize * (m_duration + m_pause));

			auto pop = m_label_pops[0];
			auto labels = mnist_helper::spikes_to_labels(
			    pop, m_duration, m_pause, m_test_batchsize);
			auto &orig_labels = std::get<1>(i);
			auto correct = mnist_helper::compare_labels(orig_labels, labels);
			m_global_correct += correct;
			m_num_images += orig_labels.size();
			m_sim_time += netw.runtime().sim;
			if (m_count_spikes) {
				for (auto &pop : m_all_pops) {
					size_t count = 0.0;
					for (auto neuron : pop) {
						count += neuron.signals().data(0).size();
					}
					global_count += count;
					local_count.emplace_back(count);
					pop_size.emplace_back(pop.size());
					pids.emplace_back(pop.pid());
				}
			}
		}

		if (m_count_spikes) {
			for (size_t i = 0; i < local_count.size(); i++) {
				global_logger().info(
				    "SNABSuite",
				    "Pop " + std::to_string(pids[i]) + " with size " +
				        std::to_string(pop_size[i]) + " fired " +
				        std::to_string(local_count[i]) + " spikes");
			}
			global_logger().info("SNABSuite", "Summ of all spikes: " +
			                                      std::to_string(global_count) +
			                                      " spikes");
		}
	}

#if SNAB_DEBUG
	std::vector<std::vector<cypress::Real>> spikes_pre;
	for (size_t i = 0; i < pre_last_pop.size(); i++) {
		spikes_pre.push_back(pre_last_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes_pre,
	                                _debug_filename("spikes_pre.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_pre.csv"), m_backend);

	spikes_pre.clear();
	for (size_t i = 0; i < m_label_pops[0].size(); i++) {
		spikes_pre.push_back(m_label_pops[0][i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes_pre,
	                                _debug_filename("spikes_label.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_label.csv"), m_backend);
#endif

	Utilities::write_vector2_to_csv(accuracies,
	                                _debug_filename("accuracies.csv"));
	Utilities::plot_1d_curve(_debug_filename("accuracies.csv"), m_backend, 0,
	                         1);
}

std::vector<std::array<cypress::Real, 4>> MnistITLLastLayer::evaluate()
{
	Real acc = Real(m_global_correct) / Real(m_num_images);
	return {std::array<cypress::Real, 4>({acc, NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>({m_sim_time, NaN(), NaN(), NaN()})};
}
}  // namespace SNAB
