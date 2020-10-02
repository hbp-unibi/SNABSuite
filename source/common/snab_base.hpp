/*
 *  SNABSuite - Spiking Neural Architecture Benchmark Suite
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

#ifndef SNABSUITE_COMMON_SNAB_BASE_HPP
#define SNABSUITE_COMMON_SNAB_BASE_HPP

#include <cypress/cypress.hpp>
#include <string>

namespace SNAB {
/**
 * @brief Virtual Base class for SNABs(Benchmarks).
 * All SNABs should have seperate building of networks, execution and an
 * evaluation tasks
 */
class SNABBase {
public:
	/**
	 * @brief Constructor which reads in platform specific config file
	 * For description of the indicator initializers look into the comment of
	 * their declaration below.
	 *
	 * @param name name of the SNAB, and therefore of the config file
	 * @param backend string containing the simulation backend
	 * @param indicator_names names of benchmark indicators
	 * @param indicator_types types...
	 * @param indicator_measures measures...
	 * @param indicator_units units...
	 * @param required_parameters list of parameters which are required in the
	 * JSON file, which should be checked before unnecessary building networks
	 * @param bench_index in case of parameters to choose from (e.g. network
	 * sizes for chip, board, wafer, ...) this is the corresponding index for
	 * all arrays in config file
	 */
	SNABBase(std::string name, std::string backend,
	         std::initializer_list<std::string> indicator_names,
	         std::initializer_list<std::string> indicator_types,
	         std::initializer_list<std::string> indicator_measures,
	         std::initializer_list<std::string> indicator_units,
	         std::initializer_list<std::string> required_parameters,
	         size_t bench_index);

	/**
	 * @brief Building the neural network for benchmarking. If you want to use
	 * an
	 * external network, you should use the first version of building (and the
	 * corresponding run function), for the member network use the second
	 * function. The implementation is contained in the first one.
	 *
	 * @param network External network SNAB/benchmark will be constructed in
	 * @return cypress::Network& Pointer to the constructed network
	 */
	virtual cypress::Network &build_netw(cypress::Network &network) = 0;

	/**
	 * @brief Calls SNABBase::build_netw with the internal network.
	 *
	 * @return cypress::Network& Pointer to the constructed network
	 */
	cypress::Network &build() { return build_netw(m_netw); };

	/**
	 * Execution of the benchmark on the simulation platform. Similar to the
	 * build function, the first function contains the implementation, while the
	 * second simply uses the first with member networks.
	 * @param network External network SNAB/benchmark will be constructed in
	 */
	virtual void run_netw(cypress::Network &network) = 0;

	/**
	 * @brief Calls SNABBase::run_netw on the internal network
	 *
	 */
	void run() { run_netw(m_netw); };

	/**
	 * @brief Returns the name of the current benchmark
	 *
	 * @return const std::__cxx11::string Name of the current benchmark
	 */
	const std::string snab_name() const { return m_snab_name; }

	/**
	 * @brief Getter for SNABSSuite::m_indicator_names
	 *
	 * @return const std::vector< std::__cxx11::string >& list of benchmark
	 * indicator names
	 */
	const std::vector<std::string> &indicator_names() const
	{
		return m_indicator_names;
	}

	/**
	 * @brief Getter for SNABSSuite::m_indicator_types
	 *
	 * @return const std::vector< std::__cxx11::string >& list of benchmark
	 * indicator types
	 */
	const std::vector<std::string> &indicator_types() const
	{
		return m_indicator_types;
	}

	/**
	 * @brief Getter for SNABSSuite::m_indicator_measures
	 *
	 * @return const std::vector< std::__cxx11::string >& list of benchmark
	 * indicator measures (their unit)
	 */
	const std::vector<std::string> &indicator_measures() const
	{
		return m_indicator_measures;
	}

	/**
	 * @brief This should contain the evaluation process and return the result
	 * in order of those in names(), types() and measures(). The array contains
	 * in this order: result, standard deviation, minimal value, maximal value.
	 * If these are not provided, use NaN() as entry.
	 */
	virtual std::vector<std::array<cypress::Real, 4>> evaluate() = 0;

	/**
	 * @brief The result of evaluation() is converted into the format used by
	 * the HBP benchmark repository
	 *
	 * @return cypress::Json Benchmark results
	 */
	cypress::Json evaluate_json();

	/**
	 * @brief Getter for the config file
	 *
	 * @return cypress::Json config file
	 */
	cypress::Json get_config() const { return m_config_file; }

	/**
	 * @brief Setting a new config file. Note that before building the new
	 * network you probably want to reset the network structure, because the old
	 * populations and results will not be deleted automatically
	 *
	 * @param json new config
	 */
	void set_config(cypress::Json json);

	/**
	 * @brief Reset the internal cypress network, therefore deleting all old
	 * populations. For example in several concurrent runs with different
	 * configurations
	 */
	void reset_network() { m_netw = cypress::Network(); }

	/**
	 * @brief Returns the state of the m_valid flag. Simulation should not be
	 * executed
	 * when valid() returns false
	 *
	 * @return bool True when benchmark is valid for execution
	 */
	bool valid() const { return m_valid; }

	/**
	 * @brief Virtual method cloning the SNAB without knowing which SNAB it is
	 */
	virtual std::shared_ptr<SNABBase> clone() = 0;

	/**
	 * @brief Overwrites the config of the backend.
	 *
	 * @param setup The new configuration of the backend
	 * @param delete_old True: Delete all old entries, False: keep entries that
	 * are not overwritten
	 */
	void overwrite_backend_config(cypress::Json setup, bool delete_old = true);

	/**
	 * @brief Get the current backend configuration
	 *
	 */
	cypress::Json get_backend_config();

	/**
	 * @brief Default destructor
	 *
	 */
	virtual ~SNABBase() {}

protected:
	/**
	 * @brief Internal spiking network which should be used by the SNAB
	 *
	 */
	cypress::Network m_netw;

	/**
	 * @brief Platform specific config file which is read in with the
	 * constructor
	 *
	 */
	cypress::Json m_config_file;

	/**
	 * @brief String which contains the name of the simulation backend
	 *
	 */
	std::string m_backend;

	/**
	 * @brief Flag which tracks whether the SNAB can be executed on the backend
	 * This can be set in config file by setting the key "invalid" for the
	 * simulator
	 */
	bool m_valid = false;

	/**
	 * @brief The name of the Benchmark/SNAB
	 */
	const std::string m_snab_name;

	/**
	 * @brief For formatting the output in the correct structure introduced in
	 * the SP9 Guidebook, the evaluation process needs the exact order of the
	 * names, types and measures of the results returned from the function
	 * SNABBase::evaluate(). indicator_names should be unique for the
	 * measurement and represent the idea behind the value
	 */
	const std::vector<std::string> m_indicator_names;

	/**
	 * @brief indicator_types can be e.g. "quality", "performance", "energy
	 * consumption". See also SNABBase::indicator_names.
	 */
	const std::vector<std::string> m_indicator_types;

	/**
	 * @brief indicator_measures should be the "type of the measurement",
	 * or what has been measures, e.g. norm, p-value, time. See also
	 * SNABBase::indicator_names.
	 */
	const std::vector<std::string> m_indicator_measures;

	/**
	 * @brief indicator_units should be the "unit of the measurement",
	 * therefore the unit of the value.
	 */
	const std::vector<std::string> m_indicator_units;

	/**
	 * @brief Beginning of the filename of all debug data (including
	 * directories)
	 *
	 * @return std::__cxx11::string "debug/[backend]/[snabname]_"
	 */
	std::string _debug_filename(const std::string append = std::string()) const;

	const size_t m_bench_index;

	/**
	 * @brief Internal check of confid, set up of backend configuration
	 *
	 * @param required_parameters_vec Vector of parameters that have to be found
	 * in the config file
	 */
	void check_config(std::vector<std::string> required_parameters_vec = {});
};

/**
 * @brief Used to indicate bad or invalid results
 */
cypress::Real NaN();
}  // namespace SNAB

#endif /* SNABSUITE_COMMON_SNAB_BASE_HPP */
