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

#pragma once

#ifndef SNABSUITE_SNABS_MNIST_HPP
#define SNABSUITE_SNABS_MNIST_HPP

#include <cypress/cypress.hpp>

#include "common/neuron_parameters.hpp"
#include "common/snab_base.hpp"
#include "mnist_mlp.hpp"

namespace SNAB {

/**
 * @brief A simple feed-forward network with densely connected layers.
 * TODO: Network definition as soon as this is fixed.
 */
class SimpleMnist : public SNABBase {
public:
	SimpleMnist(const std::string backend, size_t bench_index);
	virtual cypress::Network &build_netw(cypress::Network &netw) override;
	virtual void run_netw(cypress::Network &netw) override;
	virtual std::vector<std::array<cypress::Real, 4>> evaluate() override;
	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SimpleMnist>(m_backend, m_bench_index);
	}

protected:
	NeuronParameters m_neuro_params;  // Neuron Parameters
	std::string m_neuron_type_str;    // String containing the neuron type
	size_t m_images, m_batchsize;  // Number of images in general and per batch
	cypress::Real m_duration, m_max_freq,
	    m_pause;  // Spike data: Sample duration, Sample max freq, and pause
	              // time between two images
	bool m_poisson,
	    m_train_data;  // Use poisson or regular spiking, Use train or test data
	Real m_max_weight;  // Max weight to be scaled to

	std::vector<
	    std::pair<std::vector<std::vector<Real>>, std::vector<uint16_t>>>
	    m_batch_data;  // Spiking data for the network

	bool m_batch_parallel = true;  // Run batches parallel (in one network)

	std::vector<cypress::Network>
	    m_networks;  // Used for batches not run in parallel

	std::vector<cypress::PopulationBase> m_label_pops;  // vector of label pops

	std::string m_dnn_file = "";
	bool m_scaled_image = false;
	std::shared_ptr<MNIST::MLPBase> m_mlp;

	Real m_weights_scale_factor = 0.0;

	/**
	 * @brief Converts a prepared json to a network
	 *
	 * @param data data of the network. The repo provides a python script to
	 * create a compatible json file
	 * @param netw network object in which the deep network will be created
	 * @return number of layers
	 */
	size_t create_deep_network(cypress::Network &netw, Real max_weight = 0.0);

	/**
	 * @brief Constructor used by deriving classes
	 *
	 * @param backend backend
	 * @param bench_index benchmark index
	 * @param name Name of the derived benchmark
	 */
	SimpleMnist(const std::string backend, size_t bench_index,
	            std::string name);

	/**
	 * @brief Read config from m_config
	 *
	 */
	void read_config();

	/**
	 * @brief Build the network, maybe scale down image
	 *
	 * @param netw cypress network
	 * @return the final network
	 */
	cypress::Network &build_netw_int(cypress::Network &netw);
};

class SmallMnist : public SimpleMnist {
public:
	SmallMnist(const std::string backend, size_t bench_index)
	    : SimpleMnist(backend, bench_index, __func__)
	{
	}

	// cypress::Network &build_netw(cypress::Network &netw) override;

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SmallMnist>(m_backend, m_bench_index);
	}
};

class MnistITLLastLayer : public SimpleMnist {
public:
	MnistITLLastLayer(const std::string backend, size_t bench_index)
	    : SimpleMnist(backend, bench_index, __func__)
	{
	}

	virtual cypress::Network &build_netw(cypress::Network &netw) override;
	virtual void run_netw(cypress::Network &netw) override;
	virtual std::vector<std::array<cypress::Real, 4>> evaluate() override;

	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistITLLastLayer>(m_backend, m_bench_index);
	}

protected:
	MnistITLLastLayer(const std::string backend, size_t bench_index,
	                  std::string name)
	    : SimpleMnist(backend, bench_index, name)
	{
	}
	std::pair<std::vector<std::vector<std::vector<Real>>>,
	          std::vector<uint16_t>>
	    m_spmnist;

	bool m_positive = false;
	Real m_norm_rate_hidden = 1.0;
	Real m_norm_rate_last = 1.0;
	size_t m_global_correct = 0;
	size_t m_num_images = 0;
	Real m_sim_time = 0;
	std::string m_loss_function = "CatHinge";
	bool m_last_layer_only = true;
	size_t m_num_test_images = 10000;
	size_t m_test_batchsize = 0;
};

class MnistITL : public MnistITLLastLayer {
private:
public:
	MnistITL(const std::string backend, size_t bench_index)
	    : MnistITLLastLayer(backend, bench_index, __func__)
	{
		m_last_layer_only = false;
	}
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistITL>(m_backend, m_bench_index);
	}
};

}  // namespace SNAB
#endif
