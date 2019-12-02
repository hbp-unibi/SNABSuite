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

#include <cypress/cypress.hpp>  // Neural network frontend

#include <cypress/backend/power/power.hpp>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "common/neuron_parameters.hpp"
#include "helper_functions.cpp"
#include "mnist.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
SimpleMnist::SimpleMnist(const std::string backend, size_t bench_index)
    : SimpleMnist(backend, bench_index, __func__)
{
}

SimpleMnist::SimpleMnist(const std::string backend, size_t bench_index,
                         std::string name)
    : SNABBase(name, backend, {"accuracy", "sim_time"},
               {"quality", "performance"}, {"accuracy", "time"}, {"", "ms"},
               {"neuron_type", "neuron_params", "images", "batchsize",
                "duration", "max_freq", "pause", "poisson", "max_weight",
                "train_data", "batch_parallel", "dnn_file", "scaled_image"},
               bench_index)
{
}

void SimpleMnist::read_config()
{
	m_neuron_type_str = m_config_file["neuron_type"].get<std::string>();

	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(m_neuron_type_str),
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
}

cypress::Network &SimpleMnist::build_netw_int(cypress::Network &netw)
{
	read_config();
	auto spike_mnist = mnist_helper::read_data_to_spike(
	    m_images, m_train_data, m_duration, m_max_freq, m_poisson, m_scaled_image);
	m_batch_data = mnist_helper::create_batches(spike_mnist, m_batchsize,
	                                            m_duration, m_pause, false);

	auto kerasdata = mnist_helper::read_network(m_dnn_file, true);
	m_label_pops.clear();
	if (m_batch_parallel) {
		for (auto &i : m_batch_data) {
			mnist_helper::create_spike_source(netw, i);
			create_deep_network(kerasdata, netw, m_max_weight);
			m_label_pops.emplace_back(netw.populations().back());
		}
	}
	else {
		for (auto &i : m_batch_data) {
			m_networks.push_back(cypress::Network());
			mnist_helper::create_spike_source(m_networks.back(), i);
			create_deep_network(kerasdata, m_networks.back(), m_max_weight);
			m_label_pops.emplace_back(m_networks.back().populations().back());
		}
	}

#if SNAB_DEBUG
	Utilities::write_vector2_to_csv(std::get<0>(m_batch_data[0]),
	                                _debug_filename("spikes_input.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_input.csv"), m_backend);
#endif
	return netw;
}

cypress::Network &SimpleMnist::build_netw(cypress::Network &netw)
{
	return build_netw_int(netw);
}

void SimpleMnist::run_netw(cypress::Network &netw)
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

std::vector<std::array<cypress::Real, 4>> SimpleMnist::evaluate()
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
	Real acc = Real(global_correct) / Real(images);
	Real sim_time = m_netw.runtime().sim;
	if (!m_batch_parallel) {
		sim_time = 0.0;
		for (auto &pop : m_label_pops) {
			sim_time += pop.network().runtime().sim;
		}
	}
	return {std::array<cypress::Real, 4>({acc, NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>(
	            {sim_time, NaN(), NaN(), NaN()})};  // TODO add up simtimes
}

size_t SimpleMnist::create_deep_network(const Json &data, Network &netw,
                                        Real max_weight)
{
	size_t layer_id = netw.populations().size();
	size_t counter = 0;
	for (auto &layer : data["netw"]) {
		if (layer["class_name"].get<std::string>() == "Dense") {
			size_t size = layer["size"].get<size_t>();
			auto pop = SpikingUtils::add_population(
			    m_neuron_type_str, netw, m_neuro_params, size, "spikes");

			/*auto max = mnist_helper::max_weight(layer["weights"]);
			auto conns = mnist_helper::dense_weights_to_conn(
-			    layer["weights"], m_max_weight / max, 1.0);*/
			auto conns = mnist_helper::dense_weights_to_conn2(layer["weights"],
			                                                  max_weight,
			                                                  1.0);  // TODO

			netw.add_connection(
			    netw.populations()[layer_id - 1], pop,
			    Connector::from_list(conns),
			    ("dense_" + std::to_string(counter)).c_str());
			/*netw.add_connection(
			    netw.populations()[layer_id - 1], pop,
			    Connector::from_list(std::get<1>(conns)),
			    ("dense_in_" + std::to_string(counter)).c_str());*/

			global_logger().debug("cypress", "Dense layer detected with size " +
			                                     std::to_string(size));
			counter++;
		}
		else {
			throw std::runtime_error("Unknown layer type");
		}
		layer_id++;
	}
	return counter;
}


std::vector<std::vector<std::vector<Real>>> all_conns_to_mat(const Json &data)
{
	std::vector<std::vector<std::vector<Real>>> res;
	for (auto &layer : data["netw"]) {
		if (layer["class_name"].get<std::string>() == "Dense") {
            
            res.push_back(layer["weights"].get<std::vector<std::vector<Real>>>());
		}
		else {
			throw std::runtime_error("Unknown layer type");
		}
	}
	return res;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

cypress::Network &SmallMnist::build_netw(cypress::Network &netw)
{
	return build_netw_int(netw);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

cypress::Network &InTheLoopTrain::build_netw(cypress::Network &netw)
{
	read_config();
	m_spmnist = mnist_helper::read_data_to_spike(m_images, true, m_duration,
	                                             m_max_freq, m_poisson, m_scaled_image);
	return netw;
}

void InTheLoopTrain::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));

	auto kerasdata = mnist_helper::read_network(m_dnn_file, true);

	m_batch_data = mnist_helper::create_batches(m_spmnist, m_batchsize,
	                                            m_duration, m_pause, true);
	auto source_n = mnist_helper::create_spike_source(netw, m_batch_data[0]);
	auto n_layer = create_deep_network(kerasdata, netw, m_max_weight);
	m_label_pops = {netw.populations().back()};

	auto pre_last_pop = netw.populations()[netw.populations().size() - 2];
	pre_last_pop.signals().record(0);
	auto conn = netw.connection(
	    "dense_" +
	    std::to_string(n_layer - 1));  // last one is just inhibitory, dnn
	                                   // spikey has no inhibitory connections
                                        // TODO
	std::vector<cypress::LocalConnection> conn_list;
	conn.connect(conn_list);
	//#if SNAB_DEBUG
	std::vector<std::vector<Real>> accuracies;
	size_t counter = 0;
	//#endif
	for (size_t train_run = 0; train_run < m_config_file["epochs"];
	     train_run++) {
		m_batch_data = mnist_helper::create_batches(m_spmnist, m_batchsize,
		                                            m_duration, m_pause, true);
		for (auto &i : m_batch_data) {
			mnist_helper::update_spike_source(source_n, i);
			netw.run(pwbackend, m_batchsize * (m_duration + m_pause));

			// std::vector<std::vector<cypress::Real>> spikes;
			// auto pop = m_label_pops[0];
			/*for (size_t i = 0; i < pop.size(); i++) {
			    spikes.push_back(pop[i].signals().data(0));
			}*/
			auto errors = mnist_helper::calculate_errors(
			    mnist_helper::spikes_to_rates(m_label_pops[0], m_duration,
			                                  m_pause, std::get<1>(i).size(),
			                                  m_duration),
			    i);

			/*std::vector<std::vector<cypress::Real>> spikes_pre;
			for (size_t i = 0; i < pre_last_pop.size(); i++) {
			    spikes_pre.push_back(pre_last_pop[i].signals().data(0));
			}*/
			auto pre_rates = mnist_helper::spikes_to_rates(
			    pre_last_pop, m_duration, m_pause, m_batchsize,
			    false);  // TODO norm?
			mnist_helper::perceptron_rule(
			    errors, conn_list,
			    m_config_file["learn_rate"].get<Real>() * m_max_weight,
			    pre_rates, true, m_max_weight);
			// conn.update_connector(Connector::from_list(conn_list));
			netw.update_connection(
			    Connector::from_list(conn_list),
			    ("dense_" + std::to_string(n_layer - 1)).c_str());

			// Calculate batch accuracy
			auto labels = mnist_helper::spikes_to_labels(
			    m_label_pops[0], m_duration, m_pause, m_batchsize);
			auto &orig_labels = std::get<1>(i);
			auto correct = mnist_helper::compare_labels(orig_labels, labels);
			global_logger().debug(
			    "SNABsuite", "Batch accuracy: " +
			                     std::to_string(Real(correct) /
			                                    Real(std::get<1>(i).size())));
			//#if SNAB_DEBUG
			accuracies.emplace_back(
			    std::vector<Real>{Real(counter) / Real(m_batch_data.size()),
			                      Real(correct) / Real(std::get<1>(i).size())});
			counter++;
			//#endif
		}
	}
	m_batch_data = mnist_helper::create_batches(m_spmnist, m_images, m_duration,
	                                            m_pause, true);
	m_batchsize = m_images;

	mnist_helper::update_spike_source(source_n, m_batch_data[0]);
	netw.run(pwbackend, m_batchsize * (m_duration + m_pause));

#if SNAB_DEBUG
	std::vector<std::vector<cypress::Real>> spikes_pre;
	for (size_t i = 0; i < pre_last_pop.size(); i++) {
		spikes_pre.push_back(pre_last_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes_pre,
	                                _debug_filename("spikes_pre.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_pre.csv"), m_backend);
#endif

	Utilities::write_vector2_to_csv(accuracies,
	                                _debug_filename("accuracies.csv"));
	Utilities::plot_1d_curve(_debug_filename("accuracies.csv"), m_backend, 0,
	                         1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

cypress::Network &InTheLoopTrain2::build_netw(cypress::Network &netw)
{
	read_config();
	m_spmnist = mnist_helper::read_data_to_spike(m_images, true, m_duration,
	                                             m_max_freq, m_poisson, m_scaled_image);
	if (m_config_file.find("positive") != m_config_file.end()) {
		m_positive = m_config_file["positive"].get<bool>();
	}
	if (m_config_file.find("norm_rate_hidden") != m_config_file.end()) {
		m_norm_rate_hidden = m_config_file["norm_rate_hidden"].get<Real>();
	}
	if (m_config_file.find("m_norm_rate_last") != m_config_file.end()) {
		m_norm_rate_last = m_config_file["norm_rate_last"].get<Real>();
	}

	return netw;
}

void InTheLoopTrain2::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));

	auto kerasdata = mnist_helper::read_network(m_dnn_file, true);

	m_batch_data = mnist_helper::create_batches(m_spmnist, m_batchsize,
	                                            m_duration, m_pause, false);
	auto source_n = mnist_helper::create_spike_source(netw, m_batch_data[0]);
	create_deep_network(kerasdata, netw, m_max_weight);
	m_label_pops = {netw.populations().back()};

	for (auto pop : netw.populations()) {
		pop.signals().record(0);
	}

	auto all_weights = all_conns_to_mat(kerasdata);
	//#if SNAB_DEBUG
	std::vector<std::vector<Real>> accuracies;
	size_t counter = 0;
	//#endif
	for (size_t train_run = 0; train_run < m_config_file["epochs"];
	     train_run++) {
		m_batch_data = mnist_helper::create_batches(m_spmnist, m_batchsize,
		                                            m_duration, m_pause, true);
		for (auto &i : m_batch_data) {
			mnist_helper::update_spike_source(source_n, i);
			netw.run(pwbackend, m_batchsize * (m_duration + m_pause));

			std::vector<std::vector<std::vector<Real>>> output_rates;
			for (auto &pop : netw.populations()) {
				if (pop.pid() != netw.populations().back().pid()) {
					output_rates.emplace_back(mnist_helper::spikes_to_rates(
					    pop, m_duration, m_pause, m_batchsize, m_norm_rate_hidden));
				}
				else {
					output_rates.emplace_back(mnist_helper::spikes_to_rates(
					    pop, m_duration, m_pause, m_batchsize, m_norm_rate_last));
				}
			}

			mnist_helper::dense_backprop(
			    output_rates, all_weights, i,
			    m_config_file["learn_rate"].get<Real>(), 0.0, m_positive);

			mnist_helper::update_conns_from_mat(all_weights, netw, 1.0,
			                                    m_max_weight);

			// Calculate batch accuracy
			auto labels = mnist_helper::spikes_to_labels(
			    m_label_pops[0], m_duration, m_pause, m_batchsize);
			//auto &orig_labels = std::get<1>(i);
			auto correct = mnist_helper::compare_labels(std::get<1>(i), labels);
			global_logger().debug(
			    "SNABsuite", "Batch accuracy: " +
			                     std::to_string(Real(correct) /
			                                    Real(std::get<1>(i).size())));
			//#if SNAB_DEBUG
			accuracies.emplace_back(
			    std::vector<Real>{Real(counter) / Real(m_batch_data.size()),
			                      Real(correct) / Real(std::get<1>(i).size())});
			counter++;
			//#endif
		}
	}
	m_batch_data = mnist_helper::create_batches(m_spmnist, m_images, m_duration,
	                                            m_pause, true);
	m_batchsize = m_images;

	mnist_helper::update_spike_source(source_n, m_batch_data[0]);
	netw.run(pwbackend, m_batchsize * (m_duration + m_pause));

#if SNAB_DEBUG
	auto pre_last_pop = netw.populations()[netw.populations().size() - 2];
	std::vector<std::vector<cypress::Real>> spikes_pre;
	for (size_t i = 0; i < pre_last_pop.size(); i++) {
		spikes_pre.push_back(pre_last_pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes_pre,
	                                _debug_filename("spikes_pre.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_pre.csv"), m_backend);


	Utilities::write_vector2_to_csv(accuracies,
	                                _debug_filename("accuracies.csv"));
	Utilities::plot_1d_curve(_debug_filename("accuracies.csv"), m_backend, 0,
	                         1);
#endif
}
}  // namespace SNAB
