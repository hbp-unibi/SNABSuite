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

#include "common/snab_base.hpp"
#include "mnist_mlp.hpp"

namespace SNAB {
using namespace cypress;

/**
 * A simple feed-forward network with densely connected layers.
 * This is just the base class
 */
class MNIST_BASE : public SNABBase {
public:
	MNIST_BASE(const std::string backend, size_t bench_index);
	virtual cypress::Network &build_netw(cypress::Network &netw) override;
	virtual void run_netw(cypress::Network &netw) override;
	virtual std::vector<std::array<cypress::Real, 4>> evaluate() override;
	/*virtual std::shared_ptr<SNABBase> clone() override
	{
	    return std::make_shared<MNIST_BASE>(m_backend, m_bench_index);
	}*/

	virtual ~MNIST_BASE() = default;

protected:
	NeuronParameter m_neuro_params;  // Neuron Parameters
	std::string m_neuron_type_str;   // String containing the neuron type
	size_t m_images, m_batchsize;  // Number of images in general and per batch
	cypress::Real m_duration, m_max_freq,
	    m_pause;  // Spike data: Sample duration, Sample max freq, and pause
	              // time between two images
	bool m_poisson,
	    m_train_data;  // Use poisson or regular spiking, Use train or test data
	cypress::Real m_max_weight;  // Max weight to be scaled to
	// std::vector<cypress::Real> m_conv_max_weight;
	cypress::Real m_max_pool_weight;
	cypress::Real m_pool_inhib_weight;
	cypress::Real m_pool_delay;

	std::vector<mnist_helper::MNIST_DATA>
	    m_batch_data;  // Spiking data for the network

	bool m_batch_parallel = true;  // Run batches parallel (in one network)

	std::vector<cypress::Network>
	    m_networks;  // Used for batches not run in parallel

	std::vector<cypress::PopulationBase> m_label_pops;  // vector of label pops

	std::string m_dnn_file = "";
	bool m_scaled_image = false;
	std::shared_ptr<MNIST::MLPBase> m_mlp;
	bool m_ttfs = false;
	size_t m_activity_based_scaling = 0.0;
	std::vector<Real> m_layer_scale_factors;

	cypress::Real m_weights_scale_factor = 0.0;
	std::vector<cypress::Real> m_conv_weights_scale_factors;

	bool m_count_spikes = false;
	std::vector<cypress::PopulationBase> m_all_pops;

	/**
	 * @brief Converts a prepared json to a network
	 *
	 * @param data data of the network. The repo provides a python script to
	 * create a compatible json file
	 * @param netw network object in which the deep network will be created
	 * @return number of layers
	 */
	size_t create_deep_network(cypress::Network &netw, Real max_weight = 0.0,
	                           Real max_pool_weight = 0.0,
	                           Real pool_inhib_weight = 0.0);

	/**
	 * @brief Constructor used by deriving classes
	 *
	 * @param backend backend
	 * @param bench_index benchmark index
	 * @param name Name of the derived benchmark
	 */
	MNIST_BASE(const std::string backend, size_t bench_index, std::string name);

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

/**
 * A simple feed-forward network with densely connected layers.
 * This network has 89 x 100 x 10 layout with images downscaled by 3x3 average
 * pooling and no inhibition
 */
class MnistSpikey : public MNIST_BASE {
public:
	MnistSpikey(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistSpikey>(m_backend, m_bench_index);
	}
	~MnistSpikey() override = default;
};

/**
 * A simple feed-forward network with densely connected layers.
 * This is a network optimized by Neural Architecture Search. Layout:
 * 784 x 43 x 10 x 10
 */
class MnistNAS63 : public MNIST_BASE {
public:
	MnistNAS63(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistNAS63>(m_backend, m_bench_index);
	}
	~MnistNAS63() override = default;
};

/**
 * A simple feed-forward network with densely connected layers.
 * This is a network optimized by Neural Architecture Search. Layout:
 * 784 x 52 x 35 x 32 x 10
 */
class MnistNAS129 : public MNIST_BASE {
public:
	MnistNAS129(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistNAS129>(m_backend, m_bench_index);
	}
	~MnistNAS129() override = default;
};

/**
 * A simple feed-forward network with densely connected layers.
 * This is a network optimized by Neural Architecture Search. Layout:
 * 784 x 866 x 52 x 35 x 32 x 10
 */
class MnistNAStop : public MNIST_BASE {
public:
	MnistNAStop(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistNAStop>(m_backend, m_bench_index);
	}
	~MnistNAStop() override = default;
};

/**
 * A simple feed-forward network with densely connected layers.
 * This is a network taken from
 * https://github.com/dannyneil/spiking_relu_conversion. Corresponding paper:
 * Diehl et al.: M. Fast-Classifying, High-Accuracy Spiking Deep Networks
 * Through Weight and Threshold Balancing. Layout: 784 x 1200 x 1200 x 10
 */
class MnistDiehl : public MNIST_BASE {
public:
	MnistDiehl(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistDiehl>(m_backend, m_bench_index);
	}
	~MnistDiehl() override = default;
};

/**
 * This class combines the MNIST benchmark with an hardware in the loop
 * re-training to compensate device mismatch. Here, we train only the last layer
 */
class MnistITLLastLayer : public MNIST_BASE {
public:
	MnistITLLastLayer(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
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
	    : MNIST_BASE(backend, bench_index, name)
	{
	}
	mnist_helper::SPIKING_MNIST m_spmnist;

	bool m_positive = false;
	cypress::Real m_norm_rate_hidden = 1.0;
	cypress::Real m_norm_rate_last = 1.0;
	size_t m_global_correct = 0;
	size_t m_num_images = 0;
	cypress::Real m_sim_time = 0;
	std::string m_loss_function = "CatHinge";
	bool m_last_layer_only = true;
	size_t m_num_test_images = 10000;
	size_t m_test_batchsize = 0;
	std::vector<Real> m_time_to_sol;
};

/**
 * This class combines the MNIST benchmark with an hardware in the loop
 * re-training to compensate device mismatch. Here, we train all layers
 */
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

/**
 * This class combines the MNIST benchmark with an hardware in the loop
 * re-training to compensate device mismatch. Here, we train all layers and
 * make use of the TTFS encoding
 */
class MnistITLTTFS : public MnistITLLastLayer {
private:
public:
	MnistITLTTFS(const std::string backend, size_t bench_index)
	    : MnistITLLastLayer(backend, bench_index, __func__)
	{
		m_last_layer_only = false;
	}
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistITLTTFS>(m_backend, m_bench_index);
	}
};

/**
 * A simple feed-forward network with densely connected layers.
 * This network has 89 x 100 x 10 layout with images downscaled by 3x3 average
 * pooling and no inhibition.
 * In contrast to MnistSpikey, this SNAB uses TTFS encoding.
 */
class MnistSpikeyTTFS : public MNIST_BASE {
public:
	MnistSpikeyTTFS(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
		m_ttfs = true;
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistSpikeyTTFS>(m_backend, m_bench_index);
	}
};

/**
 * A simple feed-forward network with densely connected layers.
 * This is a network taken from
 * https://github.com/dannyneil/spiking_relu_conversion Corresponding paper:
 * Diehl et al.: M. Fast-Classifying, High-Accuracy Spiking Deep Networks
 * Through Weight and Threshold Balancing. Layout: 784 x 1200 x 1200 x 10.
 * In contrast to MnistDiehl, this SNAB uses TTFS encoding.
 */
class MnistDiehlTTFS : public MNIST_BASE {
public:
	MnistDiehlTTFS(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
		m_ttfs = true;
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistDiehlTTFS>(m_backend, m_bench_index);
	}
};

/**
 * A four layer network consisting of two convolutional and two densely
 * connected layer. Layout: conv1: 28x28x16, conv2: 26x26x16, dense1: 128,
 * dense2: 10
 */
class MnistDoubleCNN : public MNIST_BASE {
public:
	MnistDoubleCNN(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistDoubleCNN>(m_backend, m_bench_index);
	}
};

/**
 * A four layer network consisting of one convolutional, one max pooling
 * and two densely connected layer.
 * Layout:
 * conv1: 28x28x16
 * pool1: 14x14x16
 * dense1: 128
 * dense2: 10
 */
class MnistCNNPool : public MNIST_BASE {
public:
	MnistCNNPool(const std::string backend, size_t bench_index)
	    : MNIST_BASE(backend, bench_index, __func__)
	{
	}

	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<MnistCNNPool>(m_backend, m_bench_index);
	}
};

}  // namespace SNAB
#endif
