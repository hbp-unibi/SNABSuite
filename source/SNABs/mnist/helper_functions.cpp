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
#include <assert.h>

#include <chrono>
#include <cmath>
#include <cypress/cypress.hpp>
#include <fstream>
#include <string>
#include <utility>  //std::pair

#include "helper_functions.hpp"

namespace mnist_helper {
using namespace cypress;

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

std::vector<std::vector<std::vector<Real>>> image_to_rate(
    const std::vector<std::vector<Real>> &images, const Real duration,
    const Real max_freq, size_t num_images, bool poisson)
{
	std::vector<std::vector<std::vector<Real>>> rate_images;
	for (size_t i = 0; i < num_images; i++) {
		std::vector<std::vector<Real>> spike_image;
		for (const auto &pixel : images[i]) {
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

SPIKING_MNIST mnist_to_spike(const MNIST_DATA &mnist_data, const Real duration,
                             const Real max_freq, size_t num_images,
                             bool poisson)
{
	SPIKING_MNIST res;
	std::get<0>(res) = image_to_rate(std::get<0>(mnist_data), duration,
	                                 max_freq, num_images, poisson);
	std::get<1>(res) = std::get<1>(mnist_data);
	return res;
}

std::vector<MNIST_DATA> create_batches(const SPIKING_MNIST &mnist_data,
                                       const size_t batch_size, Real duration,
                                       Real pause, const bool shuffle,
                                       unsigned seed)
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

cypress::Population<SpikeSourceArray> &update_spike_source(
    cypress::Population<SpikeSourceArray> &source, const MNIST_DATA &spikes)
{
	size_t size = std::get<0>(spikes).size();

	if (source.size() != size) {
		throw std::runtime_error(
		    "Spike source array size does not equal image size!");
	}
	for (size_t nid = 0; nid < size; nid++) {
		source[nid].parameters().spike_times(std::get<0>(spikes)[nid]);
	}
	return source;
}

Json read_network(std::string path, bool msgpack)
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


std::vector<LocalConnection> dense_weights_to_conn(const Matrix<Real> &mat, Real scale,
                                   Real delay)
{
	std::vector<LocalConnection> conns;
	for (size_t i = 0; i < mat.rows(); i++) {
		for (size_t j = 0; j < mat.cols(); j++) {
			Real w = mat(i, j);
			conns.emplace_back(LocalConnection(i, j, scale * w, delay));
		}
	}
	return conns;
}

std::vector<uint16_t> spikes_to_labels(const PopulationBase &pop, Real duration,
                                       Real pause, size_t batch_size)
{
	std::vector<uint16_t> res(batch_size);
	std::vector<std::vector<uint16_t>> binned_spike_counts;
	for (const auto &neuron : pop) {
		binned_spike_counts.push_back(
		    SpikingUtils::spike_time_binning<uint16_t>(
		        -pause * 0.5, batch_size * (duration + pause) - (pause * 0.5),
		        batch_size, neuron.signals().data(0)));
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

std::vector<std::vector<Real>> spikes_to_rates(const PopulationBase pop,
                                               Real duration, Real pause,
                                               size_t batch_size, Real norm)
{
	std::vector<std::vector<Real>> res(batch_size,
	                                   std::vector<Real>(pop.size()));
	std::vector<std::vector<uint16_t>> binned_spike_counts;
	for (const auto &neuron : pop) {
		binned_spike_counts.push_back(
		    SpikingUtils::spike_time_binning<uint16_t>(
		        -pause * 0.5, batch_size * (duration + pause) - (pause * 0.5),
		        batch_size, neuron.signals().data(0)));
	}
	if (norm > 0.0) {
		for (size_t sample = 0; sample < batch_size; sample++) {
			for (size_t neuron = 0; neuron < binned_spike_counts.size();
			     neuron++) {
				res[sample][neuron] =
				    Real(binned_spike_counts[neuron][sample]) / norm;
			}
		}
	}
	else {
		for (size_t sample = 0; sample < batch_size; sample++) {
			for (size_t neuron = 0; neuron < binned_spike_counts.size();
			     neuron++) {
				res[sample][neuron] = Real(binned_spike_counts[neuron][sample]);
			}
		}
	}
	return res;
}

size_t compare_labels(std::vector<uint16_t> &label, std::vector<uint16_t> &res)
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

std::vector<Real> av_pooling_image(std::vector<Real> &image, size_t height,
                                   size_t width, size_t pooling_size)
{
	size_t new_h = std::floor(Real(height) / Real(pooling_size));
	size_t new_w = std::floor(Real(width) / Real(pooling_size));
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

MNIST_DATA scale_mnist(MNIST_DATA &data, size_t pooling_size)
{
	MNIST_DATA res;
	std::get<1>(res) = std::get<1>(data);
	auto &tar_images = std::get<0>(res);
	for (auto &image : std::get<0>(data)) {
		tar_images.emplace_back(av_pooling_image(image, 28, 28, pooling_size));
	}
	return res;
}

SPIKING_MNIST read_data_to_spike(const size_t num_images, bool train_data,
                                 const Real duration, const Real max_freq,
                                 bool poisson, bool scale_down)
{
	mnist_helper::MNIST_DATA data;
	if (train_data) {
		data = mnist_helper::loadMnistData(num_images, "train");
	}
	else {
		data = mnist_helper::loadMnistData(num_images, "t10k");
	}
	if (scale_down) {
		data = scale_mnist(data);
	}

	return mnist_helper::mnist_to_spike(data, duration, max_freq, poisson);
}

std::vector<LocalConnection> conns_from_mat(
    const cypress::Matrix<Real> &weights, Real delay, Real scale_factor)
{
	std::vector<LocalConnection> res;
	if (scale_factor > 0) {
		for (size_t i = 0; i < weights.rows(); i++) {
			for (size_t j = 0; j < weights.cols(); j++) {
				res.emplace_back(
				    LocalConnection(i, j, weights(i, j) * scale_factor, delay));
			}
		}
		return res;
	}
	for (size_t i = 0; i < weights.rows(); i++) {
		for (size_t j = 0; j < weights.cols(); j++) {
			res.emplace_back(LocalConnection(i, j, weights(i, j), delay));
		}
	}
	return res;
}

void update_conns_from_mat(const std::vector<cypress::Matrix<Real>> &weights,
                           Network &netw, Real delay, Real scale_factor)
{
	for (size_t i = 0; i < weights.size(); i++) {
		netw.update_connection(Connector::from_list(conns_from_mat(
		                           weights[i], delay, scale_factor)),
		                       ("dense_" + std::to_string(i)).c_str());
	}
}
}  // namespace mnist_helper
