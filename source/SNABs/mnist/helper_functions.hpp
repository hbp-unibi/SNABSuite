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
#include <assert.h>

#include <chrono>
#include <cmath>
#include <cypress/cypress.hpp>
#include <fstream>
#include <string>
#include <utility>  //std::pair

namespace mnist_helper {
using namespace cypress;

typedef std::pair<std::vector<std::vector<Real>>, std::vector<uint16_t>>
    MNIST_DATA;
typedef std::pair<std::vector<std::vector<std::vector<Real>>>,
                  std::vector<uint16_t>>
    SPIKING_MNIST;

/**
 * @brief Read in MNIST data from files
 *
 * @param num_data Number of images
 * @param path path to file, without end, e.g. /path/to/data/train for training
 * data, /path/to/data/t10k for test data
 * @return pair, std::get<0> is a vector of images, std::get<1> a vector of
 * labels
 */
MNIST_DATA loadMnistData(const size_t num_data, const std::string path);

/**
 * @brief Prints image to std::cout
 *
 * @param img the image
 * @param wrap line wrap -> width of the image
 */
void print_image(const std::vector<Real> &img, size_t wrap);

/**
 * @brief Converts a vector of images to a rate based representation
 *
 * @param images vector of images
 * @param duration duration of the rate
 * @param max_freq maximal rate/frequency
 * @param num_images number of images to read in
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @return vector (images) of vector (pixel) of spikes
 */
std::vector<std::vector<std::vector<Real>>> image_to_rate(
    const std::vector<std::vector<Real>> &images, const Real duration,
    const Real max_freq, size_t num_images, bool poisson = true);

/**
 * @brief Converts a vector of images to a time-to-first-spike (TTFS) based
 * representation
 *
 * @param images vector of images
 * @param duration duration of the rate
 * @param max_freq maximal rate/frequency
 * @param num_images number of images to read in
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @return vector (images) of vector (pixel) of spikes
 */
std::vector<std::vector<std::vector<Real>>> image_to_TTFS(
    const std::vector<std::vector<Real>> &images, const Real duration,
    size_t num_images);

/**
 * @brief Converts the full MNIST dataset to a spiking MNIST dataset
 *
 * @param mnist_data data container from loadMnistData
 * @param duration duration of spikes per image
 * @param max_freq Maximal rate (e.g. for px = 1)
 * @param num_images number of images to read in
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @param ttfs use time-to-first-spike encoding. Invalidates poisson.
 * @return pair, std::get<0> is a vector of spiking images, std::get<1> a vector
 * of labels
 */
SPIKING_MNIST mnist_to_spike(const MNIST_DATA &mnist_data, const Real duration,
                             const Real max_freq, size_t num_images,
                             bool poisson = true, bool ttfs = false);

/**
 * @brief Creates batches of spikes representing the MNIST data
 *
 * @param mnist_data Spiking MNIST data
 * @param batch_size number of images per batch
 * @param duration duration of every image
 * @param pause time in between images
 * @param shuffle True for shuffling images. Defaults to false.
 * @param seed Seed for shuffling images Defaults to 0.
 * @return A vector of spike batches, every vector entry gives a container.
 * std::get<0> give spikes for every pixel representing all images in one batch,
 * std::get<1> returns labels
 */
std::vector<MNIST_DATA> create_batches(const SPIKING_MNIST &mnist_data,
                                       const size_t batch_size, Real duration,
                                       Real pause, const bool shuffle = false,
                                       unsigned seed = 0);

/**
 * @brief Creates Spike sources in network from spikes
 *
 * @param netw a cypress network
 * @param spikes One batch from the return value of "create_batch"
 * @return SpikeSourceArray Population
 */
cypress::Population<SpikeSourceArray> create_spike_source(
    Network &netw, const MNIST_DATA &spikes);

/**
 * @brief Update Spike sources in network from spikes
 *
 * @param source a cypress source array
 * @param spikes One batch from the return value of "create_batch"
 * @return SpikeSourceArray Population
 */
cypress::Population<SpikeSourceArray> &update_spike_source(
    cypress::Population<SpikeSourceArray> &source, const MNIST_DATA &spikes);

/**
 * @brief Read in the network file from json of msgpack. The Repo provides a
 * script which creates compatible files
 *
 * @param path full path to file
 * @param msgpack True: Compressed msgpack. False: plain Json. Defaults to true.
 * @return The json containing the json from file
 */
Json read_network(std::string path, bool msgpack = true);

/**
 * @brief Calculate the max weight, ignore negative values
 *
 * @param json json array containing the weights
 * @return max weight
 */
template <typename T>
Real max_weight(const T &json)
{
	Real max = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		for (size_t j = 0; j < json[i].size(); j++) {
			Real tmp = Real(json[i][j]);
			if (tmp > max) {
				max = tmp;
			}
		}
	}
	return max;
}

/**
 * @brief Calculate the max weight, ignore negative values
 *
 * @param mat cypress Matrix containing the weights
 * @return max weight
 */
template <typename T>
Real max_weight(const Matrix<T> &mat)
{
	Real max = 0.0;
	for (size_t i = 0; i < mat.size(); i++) {
		Real tmp = Real(mat[i]);
		if (tmp > max) {
			max = tmp;
		}
	}
	return max;
}

/**
 * @brief Calculate the min weight
 *
 * @param json json array containing the weights
 * @return min weight
 */
template <typename T>
Real min_weight(const T &json)
{
	Real min = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		for (size_t j = 0; j < json[i].size(); j++) {
			Real tmp = Real(json[i][j]);
			if (tmp < min) {
				min = tmp;
			}
		}
	}
	return min;
}

/**
 * @brief Calculate the min weight
 *
 * @param mat cypress Matrix containing the weights
 * @return min weight
 */
template <typename T>
Real min_weight(const Matrix<T> &json)
{
	Real min = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		Real tmp = Real(json[i]);
		if (tmp < min) {
			min = tmp;
		}
	}
	return min;
}

/**
 * @brief Calculate the max absolute weight
 *
 * @param json json array containing the weights
 * @return max weight
 */
template <typename T>
Real max_weight_abs(const T &json)
{
	Real max = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		for (size_t j = 0; j < json[i].size(); j++) {
			Real tmp = fabs(Real(json[i][j]));
			if (tmp > max) {
				max = tmp;
			}
		}
	}
	return max;
}

/**
 * @brief Calculate the max absolute weight
 *
 * @param mat cypress Matrix containing the weights
 * @return max weight
 */
template <typename T>
Real max_weight_abs(const Matrix<T> &json)
{
	Real max = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		Real tmp = fabs(Real(json[i]));
		if (tmp > max) {
			max = tmp;
		}
	}
	return max;
}

/**
 * @brief Convert a dense layer to list of Local Connections.
 *
 * @param mat cypress matrix of weights
 * @param scale scale factor for weights
 * @param delay synaptic delay
 * @return vector of connections
 */
std::vector<LocalConnection> dense_weights_to_conn(const Matrix<Real> &mat,
                                                   Real scale, Real delay);

/**
 * @brief Converts the simulation results into label data
 *
 * @param pop the label population
 * @param duration presentation time of a sample
 * @param pause pause time in between samples
 * @param batch_size number of samples interpreted by these neurons (batch size)
 * @param ttfs use TTFS encoding
 * @return a vector of labels, for one hot coded neurons, if two neurons had the
 * same activation or there was no activation at all,
 * std::numeric_limits<uint16_t>::max() is returned
 */
std::vector<uint16_t> spikes_to_labels(const PopulationBase &pop, Real duration,
                                       Real pause, size_t batch_size,
                                       bool ttfs = false);

/**
 * @brief Converts the simulation results into values between 0 and 1
 *
 * @param pop the population to be converted to rates
 * @param duration presentation time of a sample
 * @param pause pause time in between samples
 * @param batch_size number of samples interpreted by these neurons (batch size)
 * @oaram norm divide the number of spikes in bin by this value, ignore if it is
 * set to zero
 * @return for every sample a vector of rates for every neuron
 * (vec[sample][neuron])
 */
std::vector<std::vector<Real>> spikes_to_rates(const PopulationBase pop,
                                               Real duration, Real pause,
                                               size_t batch_size,
                                               Real norm = 0.0);

/**
 * @brief Compare original labels with simulation labels, return number of
 * correct labels
 *
 * @param label data label
 * @param res label from simulation
 * @return number of correct labels
 */
size_t compare_labels(std::vector<uint16_t> &label, std::vector<uint16_t> &res);

/**
 * @brief Downscale an image by average pooling
 *
 * @param image the image itself
 * @param height height of the image
 * @param width width of the image
 * @param pooling_size size of the pooling window (e.g. 2)
 * @return the downscaled image
 */
std::vector<Real> av_pooling_image(std::vector<Real> &image, size_t height,
                                   size_t width, size_t pooling_size);

/**
 * @brief downscale the complete MNIST dataset
 *
 * @param data The MNIST dataset in a container
 * @return downscaled MNIST dataset in a container
 */
MNIST_DATA scale_mnist(MNIST_DATA &data, size_t pooling_size = 3);

/**
 * @brief Reads in MNIST test or train data.
 *
 * @param num_images number of images to read in
 * @param train_data true for training data, false for test
 * @param duration duration of spikes per image
 * @param max_freq Maximal rate (e.g. px = 1)
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @param scale_down Scales down the image by a factor of 3 in each dim
 * @return pair, std::get<0> is a vector of spiking images, std::get<1> a vector
 * of labels
 */
SPIKING_MNIST read_data_to_spike(const size_t num_images, bool train_data,
                                 const Real duration, const Real max_freq,
                                 bool poisson = true, bool scale_down = false);

/**
 * @brief Generate connection from given weight matrix
 *
 * @param weights the weight matrix, type cypress::Matrix<double>
 * @param delay synaptic delay, use 1.0 is unsure
 * @param scale_factor scale all weights, do not scale if set to zero
 * @return a list of connections
 */
std::vector<LocalConnection> conns_from_mat(
    const cypress::Matrix<Real> &weights, Real delay, Real scale_factor = 0.0);

/**
 * @brief Updates the connector is a given network with the weights provided
 *
 * @param weights the new weight in weights[layer](input, output)
 * @param netw the network to alter
 * @param delay synaptic delay Defaults to 1.0.
 * @param scale_factor Scales the weights during conversion, no scale if set to
 * zero
 */
void update_conns_from_mat(const std::vector<cypress::Matrix<Real>> &weights,
                           Network &netw, Real delay = 1.0,
                           Real scale_factor = 0.0);
}  // namespace mnist_helper
