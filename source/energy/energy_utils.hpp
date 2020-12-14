/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2020 Christoph Ostrau
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

#include <cypress/cypress.hpp>


namespace Energy{
using namespace cypress;

/**
 * @brief Returns the number of spikes of a population
 *
 * @param pop the population
 * @return the number of spikes
 */
size_t get_number_of_spikes_pop(const PopulationBase &pop);

/**
 * @brief Returns the number of spikes occurring in the network
 *
 * @param netw the simulated network
 * @param sources include source spikes or not
 * @return size_t number of spikes in the network
 */
size_t get_number_of_spikes(Network &netw, bool sources = true);


/**
 * @brief Calculate the number of neurons in a network
 *
 * @param netw the network
 * @param sources Include source neurons or not
 * @return the number of neurons
 */
size_t get_number_of_neurons(cypress::Network &netw, bool sources = true);

/**
 * @brief Goes through all the connections and identify learning enabled
 * synapses.
 *
 * @param netw network object
 * @return number of synapses
 */
size_t calc_number_stdp_synapses(const cypress::Network &netw);

/**
 * @brief Goes through all connections and counts the number of synaptic events:
 * Each spike is multiplied with the number of synapses that transmit this spike
 *
 * @param source population
 * @param conns list of connections
 * @param stdp Whether to count only STDP synapses or not count them
 * @return #spikes over O2O, A2A and other connectors
 */
std::tuple<size_t, size_t, size_t> calc_postsyn_spikes(
    const cypress::PopulationBase &pop,
    const std::vector<cypress::ConnectionDescriptor> &conns, bool stdp);

/**
 * @brief Find all connections the have a given population as source
 *
 * @param source_id the id of the source population
 * @param conns list of connections
 * @return list of indexes of connections
 */
std::vector<size_t> conn_ids_source(
    const size_t source_id,
    const std::vector<cypress::ConnectionDescriptor> &conns);

/**
 * @brief Perpare a json for storing measurement results. Init entries to zero.
 *
 * @return Json object containing relevant entries
 */
Json setup_energy_model();

/**
 * @brief Calculate the coefficients of the energy model after measurements have
 * been performed
 *
 * @param energy_model Json containing measurements
 */
void calculate_coefficients(cypress::Json &energy_model);

/**
 * @brief Go through a network after simulation, and approximate the energy
 * expenditure of the system.
 *
 * @param netw The network object after simulation
 * @param energy_model Json object containing coefficients of the energy model
 * @return the amount of energy used in Joule + its error estimated
 */
std::pair<double, double> calculate_energy(const cypress::Network &netw, const Json &energy_model);
}
