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
	 * Creates a population of type T and adds them to m_net
	 *
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @param rec_signal signal to be recorded by the backend
	 * @return Network containing the add Population (same as network)
	 */
	template <typename T>
	static cypress::PopulationBase add_typed_population(
	    Network &network, const NeuronParameters &neuronParams,
	    const size_t size,
	    const typename T::Signals &rec_signal =
	        typename T::Signals().record_spikes());

	/**
	 * Creates a population of type T and adds them to m_net without
	 * recording
	 *
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @return Network containing the add Population (same as network)
	 */
	template <typename T>
	static cypress::PopulationBase add_typed_population_no_record(
	    Network &network, const NeuronParameters &neuronParams,
	    const size_t size);

	/**
	 * Generates a NeuronType object from a string
	 *
	 * @param neuron_type_str string naming the neuron type
	 * @return the corresponding NeuronType object
	 */
	static const NeuronType &detect_type(std::string neuron_type_str);

	/**
	 * Runs SpikingUtils::add_typed_population, but gets a string containing the
	 * neuron type instead of a template argument
     * 
     * @param neuron_type_str string naming the neuron type
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @param record_signal string of the signal to be recorded by the backend
	 */
	static cypress::PopulationBase add_population(
	    const std::string neuron_type_str, Network &network,
	    const NeuronParameters &neuronParams, const size_t size,
	    const std::string record_signal = "spikes");

	/**
	 * Tries to run the simulation on given backend several times if backend
	 * produces an unexpected error (needed for e.g. BrainScaleS).
	 * @param network network object to simulate
	 * @param backend target of the simulation
	 * @param time simulation time
	 * @param n_trials number of trials before giving up
	 * @return true if simulation was succesful
	 */
	static bool rerun_fixed_number_trials(Network &network, Backend &backend,
	                                      Real time, size_t n_trials = 3);
};
}

#endif /* SNAB_UTIL_SPIKING_UTILS_HPP */
