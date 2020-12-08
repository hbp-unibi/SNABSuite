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
#include <algorithm>
#include <cmath>
#include <cypress/cypress.hpp>

#include "helper_functions.hpp"
namespace MNIST {
using cypress::Json;
using cypress::Matrix;
using cypress::Real;

/**
 * @brief Root Mean Squared Error
 *
 */
class MSE {
public:
	static inline Real calc_loss(const uint16_t label,
	                             const std::vector<Real> &output)
	{
		Real res = 0.0;
		for (size_t neuron = 0; neuron < output.size(); neuron++) {
			if (label == neuron) {
				res += (output[neuron] - 1.0) * (output[neuron] - 1.0);
			}
			else {
				res += output[neuron] * output[neuron];
			}
		}
		res = sqrt(res / Real(output.size()));
		return res;
	}
	static inline std::vector<Real> calc_error(const uint16_t label,
	                                           const std::vector<Real> &output)
	{
		std::vector<Real> res(output.size(), 0.0);
		for (size_t neuron = 0; neuron < output.size(); neuron++) {
			if (label == neuron) {
				res[neuron] = output[neuron] - 1.0;
			}
			else {
				res[neuron] = output[neuron] - 0.0;
			}
		}
		return res;
	}
};

/**
 * @brief Categorical hinge loss. Use if weights are restricted to be >0
 *
 */
class CatHinge {
public:
	static inline Real calc_loss(const uint16_t label,
	                             const std::vector<Real> &output)
	{
		Real res = 0.0;
		for (size_t neuron = 0; neuron < output.size(); neuron++) {
			if (label == neuron) {
				res += std::max(0.0, 1.0 - Real(output[neuron]));
			}
			else {
				res += std::max(0.0, 1.0 + Real(output[neuron]));
			}
		}
		return res;
	}
	static inline std::vector<Real> calc_error(const uint16_t label,
	                                           const std::vector<Real> &output)
	{
		// Real pos = output[label];
		std::vector<Real> vec = output;
		vec[label] = -0.0;
		auto neg_elem = std::max_element(vec.begin(), vec.end());
		auto index = std::distance(vec.begin(), neg_elem);
		auto res = std::vector<Real>(output.size(), 0.0);

		// Require that label neuron and the next most active neuron have at
		// least a difference of 1
		if ((*neg_elem) - output[label] + 1 >= 0.0) {
			res[label] = -1.0;
			if (label != index) {
				res[index] = +1;
			}
		}
		return res;
	}
};

/**
 * @brief ActivationFunction ReLU: Rectified Linear Unit
 *
 */
class ReLU {
public:
	static inline std::vector<Real> function(std::vector<Real> input)
	{
		for (auto &i : input) {
			i = std::max(0.0, i);
		}
		return input;
	}
	static inline std::vector<Real> derivative(std::vector<Real> input)
	{
		for (auto &i : input) {
			i = i >= 0 ? 1.0 : 0.0;
		}
		return input;
	}
};

/**
 * @brief Constraint for weights in neural network: No constraint.
 *
 */
class NoConstraint {
public:
	static inline void constrain_weights(std::vector<cypress::Matrix<Real>> &)
	{
	}

	void setup(std::vector<cypress::Matrix<Real>> &) {}
};

/**
 * @brief Constraint for weights in neural network: Only weights >0.
 *
 */
class PositiveWeights {
public:
	void setup(std::vector<cypress::Matrix<Real>> &) {}
	inline void constrain_weights(std::vector<cypress::Matrix<Real>> &layers)
	{
		for (auto &i : layers) {
			for (auto &j : i) {
				if (j < 0.0) {
					j = 0.0;
				}
			}
		}
	}
};

class PositiveLimitedWeights {
public:
	Real m_max = 0.0;

	void setup(std::vector<cypress::Matrix<Real>> &layers)
	{
		for (auto &layer : layers) {
			auto w = mnist_helper::max_weight(layer);
			if (w > m_max)
				m_max = w;
		}
	}

	inline void constrain_weights(std::vector<cypress::Matrix<Real>> &layers)
	{
		for (auto &i : layers) {
			for (auto &j : i) {
				if (j < 0.0) {
					j = 0.0;
				}
				if (j > m_max) {
					j = m_max;
				}
			}
		}
	}
};

/**
 * @brief Base class for Multi Layer Networks (--> currently Perceptron only).
 * Allows us to use polymorphism with templated class
 *
 */
class MLPBase {
public:
	virtual Real max_weight() const = 0;
	virtual Real min_weight() const = 0;
	virtual Real max_weight_abs() const = 0;
	virtual Real conv_max_weight() const = 0;
	virtual const size_t &epochs() const = 0;
	virtual const size_t &batchsize() const = 0;
	virtual const Real &learnrate() const = 0;
	virtual const mnist_helper::MNIST_DATA &mnist_train_set() = 0;
	virtual const mnist_helper::MNIST_DATA &mnist_test_set() = 0;
	virtual const std::vector<cypress::Matrix<Real>> &get_weights() = 0;
	virtual const std::vector<mnist_helper::CONVOLUTION_LAYER> &get_filter_weights() = 0;
	virtual const std::vector<mnist_helper::POOLING_LAYER> &get_pooling_layers() = 0;
	virtual const std::vector<size_t> &get_layer_sizes() = 0;
	virtual const std::vector<mnist_helper::LAYER_TYPE> &get_layer_types() = 0;
	virtual void scale_down_images(size_t pooling_size = 3) = 0;
	virtual inline bool correct(const uint16_t label,
	                            const std::vector<Real> &output) const = 0;
	virtual std::vector<std::vector<std::vector<Real>>> forward_path(
	    const std::vector<size_t> &indices, const size_t start) const = 0;
	virtual Real forward_path_test() const = 0;
	virtual void backward_path(
	    const std::vector<size_t> &indices, const size_t start,
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    bool last_only) = 0;
	virtual void backward_path_2(
	    const std::vector<uint16_t> &labels,
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    bool last_only = false) = 0;
	virtual size_t accuracy(
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    const std::vector<size_t> &indices, const size_t start) = 0;
	virtual void train(unsigned seed = 0) = 0;
	virtual ~MLPBase() {}
};

/**
 * @brief The standard densely connected multilayer Perceptron.
 * Template arguments provide the loss function, the activation function of
 * neurons (experimental) and a possible constraint for the weights
 *
 */
template <typename Loss = MSE, typename ActivationFunction = ReLU,
          typename Constraint = NoConstraint>
class MLP : public MLPBase {
protected:
	std::vector<cypress::Matrix<Real>> m_layers;
    std::vector<size_t> m_layer_sizes;
	std::vector<mnist_helper::CONVOLUTION_LAYER> m_filters;
	std::vector<mnist_helper::POOLING_LAYER> m_pools;
	std::vector<mnist_helper::LAYER_TYPE> m_layer_types;
	size_t m_epochs = 20;
	size_t m_batchsize = 100;
	Real learn_rate = 0.01;
	mnist_helper::MNIST_DATA m_mnist;
	mnist_helper::MNIST_DATA m_mnist_test;

	void load_data(std::string path)
	{
		m_mnist = mnist_helper::loadMnistData(60000, path + "train");
		m_mnist_test = mnist_helper::loadMnistData(10000, path + "t10k");
	}

	Constraint m_constraint;

public:
	/**
	 * @brief Constructor for random init
	 *
	 * @param layer_sizes list of #neurons beginning with input and ending with
	 * output layer
	 * @param epochs number of epochs to train
	 * @param batchsize mini batchsize before updating the weights
	 * @param learn_rate gradients are multiplied with this rate
	 * @param constrain constrains the weights during training, defaults to no
	 * constraint
	 */
	MLP(std::vector<size_t> layer_sizes, size_t epochs = 20,
	    size_t batchsize = 100, Real learn_rate = 0.01)
	    : m_layer_sizes(layer_sizes),
	      m_epochs(epochs),
	      m_batchsize(batchsize),
	      learn_rate(learn_rate)
	{
		for (size_t i = 0; i < layer_sizes.size() - 1; i++) {
			m_layers.emplace_back(
			    Matrix<Real>(layer_sizes[i], layer_sizes[i + 1]));
		}

		int seed = std::chrono::system_clock::now().time_since_epoch().count();
		auto rng = std::default_random_engine(seed);
		std::normal_distribution<Real> distribution(0.0, 1.0);
		for (auto &layer : m_layers) {
			// Kaiming init, best suited for ReLU activation functions
			auto scale = std::sqrt(2.0 / double(layer.rows()));
			for (size_t i = 0; i < layer.size(); i++) {
				layer[i] = distribution(rng) * scale;
			}
		}

		// Glorot uniform
		/*for (auto &layer : m_layers) {
		    auto limit = std::sqrt(6.0 / Real(layer.rows()+ layer.cols()));
		    std::uniform_real_distribution<Real> distribution(0, limit);
		    for (size_t i = 0; i < layer.size(); i++) {
		        layer[i] = distribution(rng);
		    }
		}*/
		try {
			load_data("");
		}
		catch (...) {
			load_data("../");
		}
		m_constraint.setup(m_layers);
	}

	/**
	 * @brief Constructs the network from json file. The repo provides python
	 * scripts to create those from a keras network
	 *
	 * @param data json object containing the network information
	 * @param epochs number of epochs to train
	 * @param batchsize mini batchsize before updating the weights
	 * @param learn_rate gradients are multiplied with this rate
	 * @param random Use structure from Json, initialize weights random if true
	 * @param constrain constrains the weights during training, defaults to no
	 * constraint
	 */
	MLP(Json &data, size_t epochs = 20, size_t batchsize = 100,
	    Real learn_rate = 0.01, bool random = false,
	    Constraint constraint = Constraint())
	    : m_epochs(epochs),
	      m_batchsize(batchsize),
	      learn_rate(learn_rate),
	      m_constraint(constraint)
	{
		int seed = std::chrono::system_clock::now().time_since_epoch().count();
		auto rng = std::default_random_engine(seed);
		std::normal_distribution<Real> distribution(0.0, 1.0);
		for (auto &layer : data["netw"]) {
			if (layer["class_name"].get<std::string>() == "Dense") {
				auto &json = layer["weights"];
				m_layers.emplace_back(
				    Matrix<Real>(json.size(), json[0].size()));
				auto &weights = m_layers.back();
				auto scale = std::sqrt(2.0 / double(weights.rows()));
				for (size_t i = 0; i < json.size(); i++) {
					for (size_t j = 0; j < json[i].size(); j++) {
						if (!random) {
							weights(i, j) = json[i][j].get<Real>();
						}
						else {
							weights(i, j) = distribution(rng) * scale;
						}
					}
				}
				//TODO: save input first mh
				// saved input always first input?
                m_layer_sizes.emplace_back(m_layers[0].rows());
                m_layer_types.push_back(mnist_helper::LAYER_TYPE::Dense);
				cypress::global_logger().debug(
				    "MNIST", "Dense layer detected with size " +
				                 std::to_string(weights.rows()) + " times " +
				                 std::to_string(weights.cols()));
			}
            //TODO: padding? mh
			else if (layer["class_name"].get<std::string>() == "Conv2D") {
                auto &json = layer["weights"];
				size_t kernel_x = json.size();
				size_t kernel_y = json[0].size();
				size_t kernel_z = json[0][0].size();
				size_t output = json[0][0][0].size();
				size_t stride = layer["stride"];
                size_t padding = layer["padding"] == "valid" ? 0 : 1;
				std::vector<size_t> input_sizes;
				std::vector<size_t> output_sizes;
                if (!layer["input_shape_x"].empty()){
                    input_sizes.push_back(layer["input_shape_x"]);
                    input_sizes.push_back(layer["input_shape_y"]);
                    input_sizes.push_back(layer["input_shape_z"]);
                } else {
					if (m_layer_types.back() == mnist_helper::LAYER_TYPE::Conv) {
						input_sizes.push_back(m_filters.back().output_sizes[0]);
						input_sizes.push_back(m_filters.back().output_sizes[1]);
						input_sizes.push_back(m_filters.back().output_sizes[2]);
					} else if (m_layer_types.back() == mnist_helper::LAYER_TYPE::Pooling) {
						input_sizes.push_back(m_pools.back().output_sizes[0]);
						input_sizes.push_back(m_pools.back().output_sizes[1]);
						input_sizes.push_back(m_pools.back().output_sizes[2]);
					} else if (m_layer_types.back() == mnist_helper::LAYER_TYPE::Dense) {
						throw std::runtime_error("Conv after Dense layer not implemented!");
					}
				}
				output_sizes.push_back((input_sizes[0] - kernel_x + 2*padding)/stride+1);
				output_sizes.push_back((input_sizes[1] - kernel_x + 2*padding)/stride+1);
				output_sizes.push_back(output);
				mnist_helper::CONVOLUTION_FILTER conv_filter(
				    kernel_x,
				    std::vector<std::vector<std::vector<Real>>>(kernel_y,
				    std::vector<std::vector<Real>>(kernel_z,
				    std::vector<Real>(output)))
				    );
				mnist_helper::CONVOLUTION_LAYER conv = {
				    conv_filter,
				    input_sizes,
				    output_sizes,
				    stride,
				    padding};
				m_filters.emplace_back(conv);
				auto &weights = m_filters.back().filter;
				//auto scale = std::sqrt(2.0 / double(weights.rows()));
				for (size_t i = 0; i < json.size(); i++){
					for (size_t j = 0; j < json[i].size(); j++){
						for (size_t k = 0; k < json[i][j].size(); k++){
							for (size_t l = 0 ; l < json[i][j][k].size(); l++){
								weights[i][j][k][l] = json[i][j][k][l].get<Real>();
							}
						}
					}
				}
				m_layer_sizes.emplace_back(output_sizes[0] * output_sizes[1] * output);
                m_layer_types.push_back(mnist_helper::LAYER_TYPE::Conv);
				cypress::global_logger().debug(
				    "MNIST", "Conv layer detected with size ("+
				        std::to_string(json.size())+","+std::to_string(json[0].size())+
				        ","+std::to_string(json[0][0].size())+","+std::to_string(json[0][0][0].size())+")");
			} else if(layer["class_name"].get<std::string>() == "MaxPooling2D"){
				std::vector<size_t> size = layer["size"];
				size_t stride = layer["stride"];
				std::vector<size_t> input_sizes;
				std::vector<size_t> output_sizes;
				//TODO: look if empty mlayertypes or not mh
				// first layer? mh
				if (m_layer_types.back() == mnist_helper::LAYER_TYPE::Conv){
                    input_sizes.push_back(m_filters.back().output_sizes[0]);
                    input_sizes.push_back(m_filters.back().output_sizes[1]);
                    input_sizes.push_back(m_filters.back().output_sizes[2]);
				} else if (m_layer_types.back() == mnist_helper::LAYER_TYPE::Pooling){
					input_sizes.push_back(m_pools.back().output_sizes[0]);
                    input_sizes.push_back(m_pools.back().output_sizes[1]);
                    input_sizes.push_back(m_pools.back().output_sizes[2]);
				}
				output_sizes.push_back((input_sizes[0] - size[0] + 2*0)/stride+1);
				output_sizes.push_back((input_sizes[1] - size[1] + 2*0)/stride+1);
				output_sizes.push_back(input_sizes[2]);
				mnist_helper::POOLING_LAYER pool = {input_sizes, output_sizes, size, stride};
                m_pools.emplace_back(pool);
                m_layer_sizes.emplace_back(output_sizes[0]*output_sizes[1]*output_sizes[2]);
				m_layer_types.emplace_back(mnist_helper::LAYER_TYPE::Pooling);
				cypress::global_logger().debug(
				    "MNIST", "Pooling layer detected with size (" +
				                 std::to_string(size[0]) + ", " + std::to_string(size[1]) +
				                ") and stride " + std::to_string(stride));
			}
			else {
				throw std::runtime_error("Unknown layer type");
			}
		}
		for (auto &layer : m_layers) {
			m_layer_sizes.emplace_back(layer.cols());
		}

		m_mnist = mnist_helper::loadMnistData(60000, "train");
		m_mnist_test = mnist_helper::loadMnistData(10000, "t10k");
		m_constraint.setup(m_layers);
	}

	/**
	 * @brief Return the largest weight in the network
	 *
	 * @return value of the weight
	 */
	Real max_weight() const override
	{
		Real max = 0.0;
		for (auto &layer : m_layers) {
			auto w = mnist_helper::max_weight(layer);
			if (w > max)
				max = w;
		}
		return max;
	}

	Real conv_max_weight() const override
	{
		Real max = 0.0;
		for (auto &layer : m_filters) {
			auto filter = layer.filter;
			for (size_t f = 0; f < layer.output_sizes[2]; f++) {
				for (size_t x = 0; x < filter.size(); x++) {
					for (size_t y = 0; y < filter[0].size(); y++) {
						for (size_t z = 0; z < filter[0][0].size(); z++) {
							max = filter[x][y][z][f] > max ? filter[x][y][z][f] : max;
						}
					}
				}
			}
		}
		return max;
	}

	/**
	 * @brief Return the smallest weight in the network
	 *
	 * @return value of the weight
	 */
	Real min_weight() const override
	{
		Real min = 0.0;
		for (auto &layer : m_layers) {
			auto w = mnist_helper::min_weight(layer);
			if (w < min)
				min = w;
		}
		return min;
	}

	/**
	 * @brief Return the largest absolute weight in the network
	 *
	 * @return value of the weight
	 */
	Real max_weight_abs() const override
	{
		Real max = 0.0;
		for (auto &layer : m_layers) {
			auto w = mnist_helper::max_weight_abs(layer);
			if (w > max)
				max = w;
		}
		return max;
	}

	const size_t &epochs() const override { return m_epochs; }
	const size_t &batchsize() const override { return m_batchsize; }
	const Real &learnrate() const override { return learn_rate; }

	/**
	 * @brief Returns reference to the train data
	 *
	 * @return const mnist_helper::MNIST_DATA&
	 */
	const mnist_helper::MNIST_DATA &mnist_train_set() override
	{
		return m_mnist;
	}
	/**
	 * @brief Returns reference to the test data
	 *
	 * @return const mnist_helper::MNIST_DATA&
	 */
	const mnist_helper::MNIST_DATA &mnist_test_set() override
	{
		return m_mnist_test;
	}

	/**
	 * @brief Return all weights in the form of weights[layer](src,tar)
	 *
	 * @return const std::vector< cypress::Matrix< cypress::Real > >&
	 */
	const std::vector<cypress::Matrix<Real>> &get_weights() override
	{
		return m_layers;
	}

	/**
	 * @brief Return all filter weights in the form of weights[x][y][depth][filter]
	 *
	 * @return const mnist_helper::Conv_TYPE &
	 */
	 // TODO: return whole conv struct? mh
	const std::vector<mnist_helper::CONVOLUTION_LAYER> &get_filter_weights() override
    {
		return m_filters;
	}

	const std::vector<mnist_helper::POOLING_LAYER> &get_pooling_layers() override
	{
		return m_pools;
	}

	/**
	 * @brief Return the number of neurons per layer
	 *
	 * @return const std::vector< size_t >&
	 */
	const std::vector<size_t> &get_layer_sizes() override
	{
		return m_layer_sizes;
	}

	const std::vector<mnist_helper::LAYER_TYPE> &get_layer_types() override
    {
		return m_layer_types;
	}

	/**
	 * @brief Scale down the whole data set, reduces the image by a given factor
	 * in every dimension
	 *
	 * @param pooling_size the factor reducing the image
	 */
	void scale_down_images(size_t pooling_size = 3) override
	{
		m_mnist = mnist_helper::scale_mnist(m_mnist, pooling_size);
		m_mnist_test = mnist_helper::scale_mnist(m_mnist_test, pooling_size);
	}

	/**
	 * @brief Implements matrix vector multiplication
	 *
	 * @param mat the matrix: mat.cols() ==! vec.size()
	 * @param vec the vector
	 * @return std::vector< cypress::Real >
	 */
	static inline std::vector<Real> mat_X_vec(const Matrix<Real> &mat,
	                                          const std::vector<Real> &vec)
	{
#ifndef NDEBUG
		assert(mat.cols() == vec.size());
#endif
		std::vector<Real> res(mat.rows(), 0.0);
		for (size_t i = 0; i < mat.rows(); i++) {
			for (size_t j = 0; j < mat.cols(); j++) {
				res[i] += mat(i, j) * vec[j];
			}
		}
		return res;
	}

	/**
	 * @brief Implements transposed matrix vector multiplication
	 *
	 * @param mat the matrix to transpose: mat.rows() ==! vec.size()
	 * @param vec the vector
	 * @return std::vector< cypress::Real > resulting vector
	 */
	static inline std::vector<Real> mat_trans_X_vec(
	    const Matrix<Real> &mat, const std::vector<Real> &vec)
	{
#ifndef NDEBUG
		assert(mat.rows() == vec.size());
#endif
		std::vector<Real> res(mat.cols(), 0.0);
		for (size_t i = 0; i < mat.cols(); i++) {
			for (size_t j = 0; j < mat.rows(); j++) {
				res[i] += mat(j, i) * vec[j];
			}
		}
		return res;
	}

	/**
	 * @brief Vector vector multiplication, component-wise
	 *
	 * @param vec1 first vector
	 * @param vec2 second vector of size vec1.size()
	 * @return std::vector< cypress::Real > resulting vector
	 */
	static inline std::vector<Real> vec_X_vec_comp(
	    const std::vector<Real> &vec1, const std::vector<Real> &vec2)
	{
#ifndef NDEBUG
		assert(vec1.size() == vec2.size());
#endif
		std::vector<Real> res(vec1.size());
		for (size_t i = 0; i < vec1.size(); i++) {
			res[i] = vec1[i] * vec2[i];
		}
		return res;
	}

	/**
	 * @brief Checks if the output of the network was correct
	 *
	 * @param label the correct neuron
	 * @param output the output of the layer
	 * @return true for correct classification
	 */
	inline bool correct(const uint16_t label,
	                    const std::vector<Real> &output) const override
	{
		auto it = std::max_element(output.begin(), output.end());
		auto out = std::distance(output.begin(), it);
		return out == label;
	}

	/**
	 * @brief Updates the weight matrix based on the error in this layer and the
	 * output of the previous layer
	 *
	 * @param mat weight matrix.
	 * @param errors error vector in this layer
	 * @param pre_output output rates of previous layer
	 * @param sample_num number of samples in this batch == mini batchsize
	 * @param learn_rate the learn rate multiplied with the gradient
	 */
	static inline void update_mat(Matrix<Real> &mat,
	                              const std::vector<Real> &errors,
	                              const std::vector<Real> &pre_output,
	                              const size_t sample_num,
	                              const Real learn_rate)
	{
#ifndef NDEBUG
		assert(mat.rows() == pre_output.size());
		assert(mat.cols() == errors.size());
#endif
		Real sample_num_r(sample_num);
		for (size_t i = 0; i < mat.rows(); i++) {
			for (size_t j = 0; j < mat.cols(); j++) {
				mat(i, j) = mat(i, j) - learn_rate * pre_output[i] * errors[j] /
				                            sample_num_r;
			}
		}
	}

	/**
	 * @brief Forward path of the network (--> inference)
	 *
	 * @param indices list of shuffled (?) indices
	 * @param start the start index, uses images indices[start] until
	 * indices[start +batchsize -1]
	 * @return std::vector< std::vector< std::vector< cypress::Real > > > the
	 * outputs of all layers, given in output[sample][layer][neuron]
	 */
	virtual std::vector<std::vector<std::vector<Real>>> forward_path(
	    const std::vector<size_t> &indices, const size_t start) const override
	{
		auto &input = std::get<0>(m_mnist);
		std::vector<std::vector<std::vector<Real>>> res;
		std::vector<std::vector<Real>> activations;
		for (auto size : m_layer_sizes) {
			activations.emplace_back(std::vector<Real>(size, 0.0));
		}
		for (size_t sample = 0; sample < m_batchsize; sample++) {
			res.emplace_back(activations);
		}

		for (size_t sample = 0; sample < m_batchsize; sample++) {
			if (start + sample >= indices.size())
				break;
			res[sample][0] = input[indices[start + sample]];
			for (size_t layer = 0; layer < m_layers.size(); layer++) {
				res[sample][layer + 1] = ActivationFunction::function(
				    mat_trans_X_vec(m_layers[layer], res[sample][layer]));
			}
		}
		return res;
	}

	/**
	 * @brief Forward path of test data
	 *
	 * @return cypress::Real the accuracy on test data
	 */
	virtual Real forward_path_test() const override
	{
		auto &input = std::get<0>(m_mnist_test);
		auto &labels = std::get<1>(m_mnist_test);
		std::vector<std::vector<Real>> activations;
		for (auto size : m_layer_sizes) {
			activations.emplace_back(std::vector<Real>(size, 0.0));
		}
		size_t sum = 0;
		for (size_t sample = 0; sample < input.size(); sample++) {
			activations[0] = input[sample];
			for (size_t layer = 0; layer < m_layers.size(); layer++) {
				activations[layer + 1] = ActivationFunction::function(
				    mat_trans_X_vec(m_layers[layer], activations[layer]));
			}
			if (correct(labels[sample], activations.back()))
				sum++;
		}

		return Real(sum) / Real(labels.size());
	}

	/**
	 * @brief implementation of backprop
	 *
	 * @param indices list of shuffled (?) indices
	 * @param start the start index, uses images indices[start] until
	 * indices[start +batchsize -1]
	 * @param activations result of forward path
	 * @param last_only true for last layer only training (Perceptron learn
	 * rule)
	 */
	virtual void backward_path(
	    const std::vector<size_t> &indices, const size_t start,
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    bool last_only = false) override
	{
#ifndef NDEBUG
		assert(m_batchsize == activations.size());
#endif
		const auto &labels = std::get<1>(m_mnist);
		const std::vector<cypress::Matrix<cypress::Real>> orig_weights =
		    m_layers;
		for (size_t sample = 0; sample < m_batchsize; sample++) {
			if (start + sample >= indices.size())
				break;
			const auto &activ = activations[sample];
			auto error = vec_X_vec_comp(
			    Loss::calc_error(labels[indices[start + sample]], activ.back()),
			    ActivationFunction::derivative(activ.back()));
			// TODO  works for ReLU only
			update_mat(m_layers.back(), error, activ[activ.size() - 2],
			           m_batchsize, learn_rate);
			if (!last_only) {
				for (size_t inv_layer = 1; inv_layer < m_layers.size();
				     inv_layer++) {
					size_t layer_id = m_layers.size() - inv_layer - 1;

					error = vec_X_vec_comp(
					    mat_X_vec(orig_weights[layer_id + 1], error),
					    ActivationFunction::derivative(activ[layer_id + 1]));
					update_mat(m_layers[layer_id], error, activ[layer_id],
					           m_batchsize, learn_rate);
				}
			}
		}
		m_constraint.constrain_weights(m_layers);
	}
	/**
	 * @brief Implementation of backprop, adapted for usage in SNNs
	 *
	 * @param labels vector containing labels of the given batch
	 * @param activations activations in the form of [layer][sample][neuron]
	 * @param last_only true for last layer only training (Perceptron learn
	 * rule)
	 */
	virtual void backward_path_2(
	    const std::vector<uint16_t> &labels,
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    bool last_only = false) override
	{
#ifndef NDEBUG
		assert(m_batchsize == activations.back().size());
#endif
		const auto orig_weights = m_layers;
		for (size_t sample = 0; sample < m_batchsize; sample++) {

			auto error = vec_X_vec_comp(
			    Loss::calc_error(labels[sample], activations.back()[sample]),
			    ActivationFunction::derivative(activations.back()[sample]));
			// TODO  works for ReLU only
			update_mat(m_layers.back(), error,
			           activations[activations.size() - 2][sample], m_batchsize,
			           learn_rate);
			if (!last_only) {
				for (size_t inv_layer = 1; inv_layer < m_layers.size();
				     inv_layer++) {
					size_t layer_id = m_layers.size() - inv_layer - 1;

					error = vec_X_vec_comp(
					    mat_X_vec(orig_weights[layer_id + 1], error),
					    ActivationFunction::derivative(
					        activations[layer_id + 1][sample]));
					update_mat(m_layers[layer_id], error,
					           activations[layer_id][sample], m_batchsize,
					           learn_rate);
				}
			}
			m_constraint.constrain_weights(m_layers);
		}
	}

	/**
	 * @brief Calculate the overall accuracy from the given neural network
	 * output
	 *
	 * @param activations output of forward path
	 * @param start the start index, uses images indices[start] until
	 * indices[start +batchsize -1]
	 * @param activations result of forward path
	 * @return the number of correctly classified images
	 */
	size_t accuracy(
	    const std::vector<std::vector<std::vector<Real>>> &activations,
	    const std::vector<size_t> &indices, const size_t start) override
	{
#ifndef NDEBUG
		assert(activations.size() == m_batchsize);
#endif

		auto &labels = std::get<1>(m_mnist);
		size_t sum = 0;

		for (size_t sample = 0; sample < m_batchsize; sample++) {
			if (start + sample >= indices.size())
				break;
			if (correct(labels[indices[start + sample]],
			            activations[sample].back()))
				sum++;
		}
		return sum;
	}

	/**
	 * @brief Starts the full training process
	 *
	 * @param seed sets up the random numbers for image shuffling
	 */
	void train(unsigned seed = 0) override
	{
		std::vector<size_t> indices(std::get<0>(m_mnist).size());
		m_constraint.constrain_weights(m_layers);
		for (size_t i = 0; i < indices.size(); i++) {
			indices[i] = i;
		}
		if (seed == 0) {
			seed = std::chrono::system_clock::now().time_since_epoch().count();
		}
		auto rng = std::default_random_engine{seed};

		for (size_t epoch = 0; epoch < m_epochs; epoch++) {
			size_t correct = 0;
			std::shuffle(indices.begin(), indices.end(), rng);
			for (size_t current_idx = 0;
			     current_idx < std::get<1>(m_mnist).size();
			     current_idx += m_batchsize) {
				auto activations = forward_path(indices, current_idx);
				correct += accuracy(activations, indices, current_idx);
				backward_path(indices, current_idx, activations);
				m_constraint.constrain_weights(m_layers);
			}
			cypress::global_logger().info(
			    "MLP", "Accuracy of epoch " + std::to_string(epoch) + ": " +
			               std::to_string(Real(correct) /
			                              Real(std::get<1>(m_mnist).size())));
		}
	}
};
}  // namespace MNIST
