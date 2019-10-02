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

#include <memory>
#include <random>
#include <string>
#include <vector>

#include <cypress/backend/power/power.hpp>
#include "common/neuron_parameters.hpp"
#include "helper_functions.cpp"
#include "mnist.hpp"
#include "util/spiking_utils.hpp"
#include "util/utilities.hpp"

namespace SNAB {
SimpleMnist::SimpleMnist(const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend, {""}, {""}, {""}, {""},
               {
                   "neuron_type",
                   "neuron_params",
               },
               bench_index)
{
}

cypress::Network &SimpleMnist::build_netw(cypress::Network &netw)
{
	m_neuron_type_str = m_config_file["neuron_type"].get<std::string>();

	m_neuro_params =
	    NeuronParameters(SpikingUtils::detect_type(m_neuron_type_str),
	                     m_config_file["neuron_params"]);

	m_neuro_params.set("tau_syn_I", m_neuro_params.get("tau_syn_E"));

	m_images = m_config_file["images"].get<size_t>();
	m_batchsize = m_config_file["batchsize"].get<size_t>();
	m_duration = m_config_file["duration"].get<Real>();
	m_max_freq = m_config_file["max_freq"].get<Real>();
	m_pause = m_config_file["pause"].get<Real>();
	m_poisson = m_config_file["poisson"].get<bool>();
	m_max_weight = m_config_file["max_weight"].get<Real>();
	auto data = mnist_helper::loadMnistData(m_images, "t10k");

	auto spike_mnist =
	    mnist_helper::mnist_to_spike(data, m_duration, m_max_freq, m_poisson);
	m_batch_data = mnist_helper::create_batch(spike_mnist, m_batchsize,
	                                          m_duration, m_pause, false);

	mnist_helper::create_spike_source(netw, m_batch_data[0]);

#if SNAB_DEBUG
	Utilities::write_vector2_to_csv(std::get<0>(batch_data[0]),
	                                _debug_filename("spikes_input.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_input.csv"), m_backend);
#endif

	auto kerasdata = mnist_helper::read_network(
	    "../source/SNABs/mnist/python/test.msgpack", true);
	create_deep_network(kerasdata, netw);
	return netw;
}

void SimpleMnist::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	try {
		netw.run(pwbackend, m_batchsize * (m_duration + m_pause));
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

	std::vector<std::vector<cypress::Real>> spikes;
	auto pop = m_netw.populations().back();
	for (size_t i = 0; i < pop.size(); i++) {
		spikes.push_back(pop[i].signals().data(0));
	}
	auto labels = mnist_helper::spikes_to_labels(spikes, m_duration, m_pause,
	                                             m_batchsize);
	auto &orig_labels = std::get<1>(m_batch_data[0]);
	auto correct = mnist_helper::compare_labels(orig_labels, labels);
	Real accuracy = Real(correct) / Real(orig_labels.size());

#if SNAB_DEBUG
	std::cout << "Target\t Infer" << std::endl;
	for (size_t i = 0; i < labels.size(); i++) {
		std::cout << orig_labels[i] << "\t" << labels[i] << std::endl;
	}
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes.csv"));
	Utilities::plot_spikes(_debug_filename("spikes.csv"), m_backend);
#endif
	return {std::array<cypress::Real, 4>({accuracy, NaN(), NaN(), NaN()})};
}

void SimpleMnist::create_deep_network(const Json &data, Network &netw)
{
	size_t layer_id = 1;
	for (auto &layer : data["netw"]) {
		if (layer["class_name"].get<std::string>() == "Dense") {
			size_t size = layer["size"].get<size_t>();
			auto pop = SpikingUtils::add_population(
			    m_neuron_type_str, netw, m_neuro_params, size, "spikes");

			auto max = mnist_helper::max_weight(layer["weights"]);

			auto conns = mnist_helper::dense_weights_to_conn(
			    layer["weights"], m_max_weight / max, 1.0);
			netw.add_connection(netw.populations()[layer_id - 1], pop,
			                    Connector::from_list(std::get<0>(conns)));
			netw.add_connection(netw.populations()[layer_id - 1], pop,
			                    Connector::from_list(std::get<1>(conns)));

			global_logger().debug("cypress", "Dense layer detected");
		}
		else {
			throw std::runtime_error("Unknown layer type");
		}
		layer_id++;
	}
}

}  // namespace SNAB
