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

#include <cypress/cypress.hpp>

#include <chrono>
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
typedef std::pair<std::vector<LocalConnection>, std::vector<LocalConnection>>
    CYPRESS_CONN;

/**
 * @brief Read in MNIST data from files
 *
 * @param num_data Number of images
 * @param path path to file, without end, e.g. /path/to/data/train for training
 * data, /path/to/data/t10k for test data
 * @return pair, std::get<0> is a vector of images, std::get<1> a vector of
 * labels
 */
MNIST_DATA loadMnistData(const size_t num_data, const std::string path)
{
	MNIST_DATA res;
	std::ifstream images, labels;
	images.open(path + "-images-idx3-ubyte", std::ios::binary);
	if (!images.good()) {
		throw std::runtime_error("Could not open image file " + path +
		                         "-images-idx3-ubyte!");
	}
	labels.open(path + "-labels-idx1-ubyte", std::ios::binary);
	if (!images.good()) {
		throw std::runtime_error("Could not open label file " + path +
		                         "-labels-idx1-ubyte");
	}

	images.seekg(16, images.beg);
	unsigned char tmp = 0;
	for (size_t i = 0; i < num_data; i++) {
		std::vector<Real> image;
		for (size_t row = 0; row < 28; row++) {
			for (size_t col = 0; col < 28; col++) {
				images.read((char *)&tmp, sizeof(tmp));
				if (images.eof()) {
					throw std::runtime_error("Error reading file!");
				}
				image.push_back(((Real)tmp) / 255.0);
			}
		}
		std::get<0>(res).push_back(image);
	}

	labels.seekg(8, labels.beg);
	for (size_t i = 0; i < num_data; i++) {
		labels.read((char *)&tmp, sizeof(tmp));
		std::get<1>(res).push_back(uint16_t(tmp));
	}

	images.close();
	labels.close();

	return res;
}

/**
 * @brief Prints image to std::cout
 *
 * @param img the image
 * @param wrap line wrap -> width of the image
 */
void print_image(const std::vector<Real> &img, size_t wrap)
{
	size_t count = 0;
	for (auto i : img) {
		if (i > 0.5) {
			std::cout << "#";
		}
		else {
			std::cout << " ";
		}
		count++;
		if (count == wrap) {
			std::cout << std::endl;
			count = 0;
		}
	}
}

/**
 * @brief Converts a vector of images to a rate based representation
 *
 * @param images vector of images
 * @param duration duration of the rate
 * @param max_freq maximal rate/frequency
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @return vector (images) of vector (pixel) of spikes
 */
std::vector<std::vector<std::vector<Real>>> image_to_rate(
    const std::vector<std::vector<Real>> &images, const Real duration,
    const Real max_freq, bool poisson = true)
{
	std::vector<std::vector<std::vector<Real>>> rate_images;
	for (const auto &image : images) {
		std::vector<std::vector<Real>> spike_image;
		for (const auto &pixel : image) {
			if (poisson) {
				spike_image.emplace_back(
				    cypress::spikes::poisson(0.0, duration, max_freq * pixel));
			}
			else {
				spike_image.emplace_back(cypress::spikes::constant_frequency(
				    0.0, duration, max_freq * pixel));
			}
		}
		rate_images.emplace_back(spike_image);
	}
	return rate_images;
}

/**
 * @brief Converts the full MNIST dataset to a spiking MNIST dataset
 *
 * @param mnist_data data container from loadMnistData
 * @param duration duration of spikes per image
 * @param max_freq Maximal rate (e.g. px = 1)
 * @param poisson False: regular spiking. True: poisson rates. Defaults to true.
 * @return pair, std::get<0> is a vector of spiking images, std::get<1> a vector
 * of labels
 */
SPIKING_MNIST mnist_to_spike(const MNIST_DATA &mnist_data, const Real duration,
                             const Real max_freq, bool poisson = true)
{
	SPIKING_MNIST res;
	std::get<0>(res) =
	    image_to_rate(std::get<0>(mnist_data), duration, max_freq, poisson);
	std::get<1>(res) = std::get<1>(mnist_data);
	return res;
}

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
                                       unsigned seed = 0)
{
	std::vector<size_t> indices(std::get<0>(mnist_data).size());
	for (size_t i = 0; i < indices.size(); i++) {
		indices[i] = i;
	}
	if (shuffle) {
		if (seed == 0) {
			seed = std::chrono::system_clock::now().time_since_epoch().count();
		}
		auto rng = std::default_random_engine{seed};
		std::shuffle(indices.begin(), indices.end(), rng);
	}
	size_t counter = 0;
	size_t image_size = std::get<0>(mnist_data)[0].size();
	std::vector<MNIST_DATA> res;
	while (counter < indices.size()) {
		MNIST_DATA single_batch_combined;

		std::vector<std::vector<Real>> &single_batch =
		    std::get<0>(single_batch_combined);
		for (size_t pixel = 0; pixel < image_size; pixel++) {
			std::vector<Real> spike_pxl;
			for (size_t index = 0; index < batch_size; index++) {
				if (counter + index >= indices.size()) {
					break;
				}
				size_t shfld_index = indices[counter + index];

				auto pxl_spk = std::get<0>(mnist_data)[shfld_index][pixel];
				std::for_each(pxl_spk.begin(), pxl_spk.end(),
				              [&duration, &pause, &index](Real &d) {
					              d += (duration + pause) * index;
				              });
				spike_pxl.insert(spike_pxl.end(), pxl_spk.begin(),
				                 pxl_spk.end());
			}
			single_batch.emplace_back(spike_pxl);
		}
		std::vector<uint16_t> &labels = std::get<1>(single_batch_combined);
		for (size_t index = 0; index < batch_size; index++) {
			if (counter + index >= indices.size()) {
				break;
			}
			size_t shfld_index = indices[counter + index];
			labels.emplace_back(std::get<1>(mnist_data)[shfld_index]);
		}
		res.push_back(std::move(single_batch_combined));
		counter += batch_size;
	}
	return res;
}

/**
 * @brief Creates Spike sources in network from spikes
 *
 * @param netw a cypress network
 * @param spikes One batch from the return value of "create_batch"
 * @return SpikeSourceArray Population
 */
cypress::Population<SpikeSourceArray> create_spike_source(
    Network &netw, const MNIST_DATA &spikes)
{
	size_t size = std::get<0>(spikes).size();

	auto pop = netw.create_population<SpikeSourceArray>(
	    size, SpikeSourceArrayParameters(), SpikeSourceArraySignals(),
	    "input_layer");
	for (size_t nid = 0; nid < size; nid++) {
		pop[nid].parameters().spike_times(std::get<0>(spikes)[nid]);
	}
	return pop;
}

/**
 * @brief Read in the network file from json of msgpack. The Repo provides a
 * script which creates compatible files
 *
 * @param path full path to file
 * @param msgpack True: Compressed msgpack. False: plain Json. Defaults to true.
 * @return The json containing the json from file
 */
Json read_network(std::string path, bool msgpack = true)
{
	std::ifstream file_in;
	Json json;
	if (msgpack) {
		file_in.open(path, std::ios::binary);
		if (!file_in.good()) {
			throw std::runtime_error("Could not open deep network file " +
			                         path);
		}
		json = Json::from_msgpack(file_in);
	}
	else {
		file_in.open(path, std::ios::binary);
		if (!file_in.good()) {
			throw std::runtime_error("Could not open deep network file " +
			                         path);
		}
		json = Json::parse(file_in);
	}
	return json;
}

/**
 * @brief Calculate the max weight, ignore negative values
 *
 * @param json json array containing the weights
 * @return max weight
 */
Real max_weight(const Json &json)
{
	Real max = 0.0;
	for (size_t i = 0; i < json.size(); i++) {
		for (size_t j = 0; j < json[i].size(); j++) {
			Real tmp = json[i][j].get<Real>();
			if (tmp > max) {
				max = tmp;
			}
		}
	}
	return max;
}

/**
 * @brief Convert a dense layer to list of Local Connections.
 *
 * @param json json matrix of weights
 * @param scale scale factor for weights
 * @param delay synaptic delay
 * @return container of connections, std::get<0> are exc. connections,
 * std::get<1> are inhib. connections
 */
CYPRESS_CONN dense_weights_to_conn(const Json &json, Real scale, Real delay)
{
	CYPRESS_CONN conns;
	auto &conns_ex = std::get<0>(conns);
	auto &conns_in = std::get<1>(conns);
	for (size_t i = 0; i < json.size(); i++) {
		for (size_t j = 0; j < json[i].size(); j++) {
			Real w = json[i][j].get<Real>();
			if (w > 0) {
				conns_ex.emplace_back(LocalConnection(i, j, scale * w, delay));
			}
			else if (w < 0) {
				conns_in.emplace_back(LocalConnection(i, j, scale * w, delay));
			}
		}
	}
	return conns;
}

/**
 * @brief Converts the simulation results into label data
 *
 * @param pop_spikes the spikes of a populations
 * @param duration presentation time of a sample
 * @param pause pause time in between samples
 * @param batch_size number of samples interpreted by these neurons (batch size)
 * @return a vector of labels, for one hot coded neurons, if two neurons had the
 * same activation or there was no activation at all,
 * std::numeric_limits<uint16_t>::max() is returned
 */
std::vector<uint16_t> spikes_to_labels(
    const std::vector<std::vector<Real>> &pop_spikes, Real duration, Real pause,
    size_t batch_size)
{
	std::vector<uint16_t> res(batch_size);
	std::vector<std::vector<uint16_t>> binned_spike_counts;
	for (const auto &spikes : pop_spikes) {
		binned_spike_counts.push_back(
		    SpikingUtils::spike_time_binning<uint16_t>(
		        -pause * 0.5, batch_size * (duration + pause) - (pause * 0.5),
		        batch_size, spikes));
	}

	for (size_t sample = 0; sample < batch_size; sample++) {
		uint16_t max = 0;
		uint16_t index = std::numeric_limits<uint16_t>::max();
		for (size_t neuron = 0; neuron < binned_spike_counts.size(); neuron++) {
			if (binned_spike_counts[neuron][sample] > max) {
				index = neuron;
				max = binned_spike_counts[neuron][sample];
			}
			else if (binned_spike_counts[neuron][sample] == max) {
				// Multiple neurons have the same decision
				index = std::numeric_limits<uint16_t>::max();
			}
		}
		res[sample] = index;
	}
	return res;
}

/**
 * @brief Compare original labels with simulation labels, return number of
 * correct labels
 *
 * @param label data label
 * @param res label from simulation
 * @return number of correct labels
 */
size_t compare_labels(std::vector<uint16_t> label, std::vector<uint16_t> res)
{
	size_t count_correct = 0;
	if (label.size() > res.size()) {
		throw std::runtime_error("label data has incorrect size! Target: " +
		                         std::to_string(label.size()) +
		                         " Result: " + std::to_string(res.size()));
	}
	for (size_t i = 0; i < label.size(); i++) {
		if (label[i] == res[i]) {
			count_correct++;
		}
	}
	return count_correct;
}

/**
 * @brief Downscale an image by average pooling
 *
 * @param image the image itself
 * @param height height of the image
 * @param width width of the image
 * @param pooling_size size of the pooling window (e.g. 2)
 * @return the downscaled image
 */
std::vector<Real> av_pooling_image(std::vector<Real> image, size_t height,
                                   size_t width, size_t pooling_size)
{
	size_t new_h = std::ceil(Real(height) / Real(pooling_size));
	size_t new_w = std::ceil(Real(width) / Real(pooling_size));
	std::vector<Real> res(new_h * new_w, 0.0);

	for (size_t h = 0; h < new_h; h++) {
		for (size_t w = 0; w < new_w; w++) {
			std::vector<Real> vals(pooling_size * pooling_size, 0.0);
			for (size_t h2 = 0; h2 < pooling_size; h2++) {
				for (size_t w2 = 0; w2 < pooling_size; w2++) {
					if ((h * pooling_size + h2) < height &&
					    (w * pooling_size + w2) < width) {
						vals[h2 * pooling_size + w2] =
						    image[(h * pooling_size + h2) * width +
						          w * pooling_size + w2];
					}
					else {
						vals[h2 * pooling_size + w2] = 0.0;
					}
				}
			}
			// res[h * new_w + w] = *std::max_element(vals.begin(), vals.end());
			res[h * new_w + w] =
			    std::accumulate(vals.begin(), vals.end(), 0.0) /
			    Real(vals.size());
		}
	}
	return res;
}

/**
 * @brief downscale the complete MNIST dataset
 *
 * @param data The MNIST dataset in a container
 * @return downscaled MNIST dataset in a container
 */
MNIST_DATA scale_mnist(MNIST_DATA &data)
{
	MNIST_DATA res;
	std::get<1>(res) = std::get<1>(data);
	auto &tar_images = std::get<0>(res);
	for (auto &image : std::get<0>(data)) {
		tar_images.emplace_back(av_pooling_image(image, 28, 28, 2));
	}
	return res;
}

}  // namespace mnist_helper
