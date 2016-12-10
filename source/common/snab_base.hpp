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

#include "util/read_json.hpp"

namespace SNAB {
/**
 * Virtual Base class for SNABs(Benchmarks).
 * All SNABs should have seperate building of networks, execution and an
 * evaluation tasks
 */
class SNABBase {
protected:
	// Internal spiking network which should be used by the SNAB
	cypress::Network m_netw;
	// Platform specific config file which is read in with the constructor
	cypress::Json m_config_file;
	// String which contains the name of the simulation backend
	std::string m_backend;
	// Flag which tracks if the SNAB can be executed on the backend
	// In the config file define the key "invalid" for the simulator
	bool m_valid = false;

public:
	/**
	 * Constructor which reads in platform specific config file
	 * For description of the indicator_* initializers look into the comment of
	 * their declaration below
	 * @param name: name of the SNAB, and therefore of the config file
	 * @param backend: string containing the simulation backend
	 * @param indicator_names: names of benchmark indicators
	 * @param indicator_types: types...
	 * @param indicator_measures: measures...
	 * @param required_parameters: list of parameters which are required in the
	 * JSON file, which should be checked before unnecessary building networks
	 */
	SNABBase(std::string name, std::string backend,
	         std::initializer_list<std::string> indicator_names,
	         std::initializer_list<std::string> indicator_types,
	         std::initializer_list<std::string> indicator_measures,
	         std::initializer_list<std::string> required_parameters)
	    : m_backend(backend),
	      snab_name(name),
	      indicator_names(indicator_names),
	      indicator_types(indicator_types),
	      indicator_measures(indicator_measures)
	{
		m_config_file = read_config(name, m_backend);
		std::vector<std::string> required_parameters_vec(required_parameters);
		bool required_params = check_json_for_parameters(
		    required_parameters_vec, m_config_file, name);
		if ((m_config_file.find("invalid") == m_config_file.end() ||
		     m_config_file["invalid"] == false) &&
		    required_params) {
			m_valid = true;
		}
	};

	/**
	 * Building the neural network for benchmarking. If you want to use an
	 * external network, you should use the first version of building (and the
	 * corresponding run function), for the member network use the second
	 * function. The implementation is contained in the first one.
	 * @param network: External network SNAB/benchmark will be constructed in
	 */
	virtual cypress::Network &build_netw(cypress::Network &netw) = 0;
	cypress::Network &build() { return build_netw(m_netw); };

	/**
	 * Execution of the benchmark on the simulation platform. Similar to the
	 * build function, the first function contains the implementation, while the
	 * second simply uses the first with member networks.
	 * @param network: External network SNAB/benchmark will be constructed in
	 */
	virtual void run_netw(cypress::Network &netw) = 0;
	void run() { run_netw(m_netw); };

	/**
	 * The name of the Benchmark/SNAB
	 */
	const std::string snab_name;

	/**
	 * For formatting the output in the correct structure introduced in the SP9
	 * Guidebook, the evaluation process needs the exact order of the names,
	 * types
	 * and measures of the results returned from the function @evaluate().
	 * 'indicator_names' should be unique for the measurement and represent the
	 * idea of the value
	 * 'indicator_types' can be e.g. "quality", "performance", "energy
	 * consumption"
	 * 'indicator_measures' should be the "type of the measurement", therefore
	 * the unit of the value
	 * @param i: enumerates the names, measures and types. For the same value
	 * these should be related
	 */
	const std::vector<std::string> indicator_names;
	const std::vector<std::string> indicator_types;
	const std::vector<std::string> indicator_measures;

	/**
	 * This should contain the evaluation process and return the result in order
	 * of those in names(), types() and measures()
	 */
	virtual std::vector<cypress::Real> evaluate() = 0;

	/**
	 * The result of evaluation() is converted into the format used by the HBP
	 * benchmark repository
	 */
	cypress::Json evaluate_json()
	{
		std::vector<cypress::Real> results = evaluate();
		cypress::Json json;
		for (size_t i = 0; i < results.size(); i++) {
			json.push_back({{"type", indicator_types[i]},
			                {"name", indicator_names[i]},
			                {"value", results[i]},
			                {"measures", indicator_measures[i]}});
		}
		return json;
	}

	/**
	 * Returns the state of the m_valid flag. Simulation should not be executed
	 * when valid() returns false
	 */
	bool valid() const { return m_valid; }
	virtual ~SNABBase() {}
};
}

#endif /* SNABSUITE_COMMON_SNAB_BASE_HPP */
