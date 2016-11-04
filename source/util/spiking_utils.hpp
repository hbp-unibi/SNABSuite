/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2016  Christoph Jenzen
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

#ifndef SNAB_UTIL_SPIKING_UTILS_HPP
#define SNAB_UTIL_SPIKING_UTILS_HPP

#include <cypress/cypress.hpp>
#include "common/neuron_parameters.hpp"

namespace SNAB {
using namespace cypress;
class SpikingUtils {
public:
	/**
	 * Creates a population of type @param T and adds them to m_net
	 */
	template <typename T>
	static cypress::PopulationBase add_typed_population(
	    Network &network, NeuronParameters &neuronParams, size_t size);

	/**
	 * Runs add_typed_population, but gets a string containing the neuron type
	 * instead of a template argument
	 */
	static cypress::PopulationBase add_population(
	    std::string &neuron_type_str, Network &network,
	    NeuronParameters &neuronParams, size_t size);

	static const NeuronType &detect_type(std::string neuron_type_str);
};
}

#endif /* SNAB_UTIL_SPIKING_UTILS_HPP */
