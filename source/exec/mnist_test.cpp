#include <algorithm>
#include <iostream>
#include <vector>

#include "SNABs/mnist/helper_functions.hpp"
#include "SNABs/mnist/mnist_mlp.hpp"
#include "util/utilities.hpp"

using MLP = MNIST::MLP<MNIST::MSE, MNIST::ReLU>;
int main(int argc, const char *argv[])
{
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <network_file>" << std::endl;
		throw;
	}

	std::string path(argv[1]);
    
    // Load a network created with source/SNABs/mnist/python/convert_weights.py
	cypress::Json kerasdata;
	if (SNAB::Utilities::split(path, '.').back() == "msgpack") {
		kerasdata = mnist_helper::read_network(path, true);
	}
	else {
		kerasdata = mnist_helper::read_network(path, false);
	}

	MLP mlp(kerasdata, 1, 10); // Network, #epoch (irrelevant), batchsize
    
    // Which images to use for inference, indices.size()>=batchsize!
	std::vector<size_t> indices({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    // The actual forward_path
	auto data = mlp.forward_path(indices, 0);

	for (size_t sample = 0; sample < data.size(); sample++) {
		std::cout << "Output layer for sample " << sample << ":" << std::endl;
        // data[sample][layer][neuron]
		for (auto &output : data[sample].back()) {
			std::cout << output << ", ";
		}
		std::cout << std::endl;
	}

	const auto &images = mlp.mnist_train_set(); // Mnist Data
	std::cout << "Target\tCurrent" << std::endl;
	for (size_t sample = 0; sample < data.size(); sample++) {
        // Pointer to the maximal output --> The winner neuron of this sample
		auto result = std::max_element(data[sample].back().begin(),
		                               data[sample].back().end());

        // std::distance calulates the index the pointer result is pointing to
		std::cout << std::get<1>(images)[sample] << "\t"
		          << std::distance(data[sample].back().begin(), result)
		          << std::endl;
	}
}
