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

#ifndef SNABSUITE_COMMON_BENCHMARK_BASE_HPP
#define SNABSUITE_COMMON_BENCHMARK_BASE_HPP

#include <cypress/cypress.hpp>
#include <string>

#include "util/read_json.hpp"

namespace SNAB {
/**
 * Virtual Base class for SNABs/Benchmarks.
 * All Benchmarks should have seperate building of networks, execution and an
 * evaluation tasks
 */
class BenchmarkBase {
protected:
	// Internal spiking network which should be used by the benchmark
	cypress::Network m_netw;
	// PLatform specific platform file which is read in with the constructor
	cypress::Json m_config_file;
	// String which contains the name of the simulation backend
	std::string m_backend;

public:
	/**
	 * Constructor reads in platform specific config file
	 * @param name: name of the benchmark, and therefore of the config file
	 * @param backend: string containing the simulation backend
	 */
	BenchmarkBase(std::string name, std::string backend) : m_backend(backend)
	{
		m_config_file = read_config(name, m_backend);
	};

	/**
	 * Building the neural network for benchmarking. If you want to use an
	 * external network, you should use the first version of building (and the
	 * corresponding run function), for the member network use the second
	 * function. The implementation is contained in the first one.
	 * @param network: External network benchmark will be constructed in
	 */
	virtual cypress::Network &build_netw(cypress::Network &netw) = 0;
	cypress::Network &build() { return build_netw(m_netw); };

	/**
	 * Execution of the benchmark on the simulation platform. Similar to the
	 * buil function, the first function contains the implementation, while the
	 * second simply uses the first with member networks.
	 * @param network: External network benchmark will be constructed in
	 */
	virtual void run_netw(cypress::Network &netw) = 0;
	void run() { run_netw(m_netw); };

	/**
	 * For formatting the output in the correct structure introduce in the SP9
	 * Guidebook, the evaluation process needs the exact order of the names,
	 * types
	 * and measures of the results returned from the fuction @evaluate().
	 * 'Names' should be unique for the measurment represent the idea of the
	 * value
	 * 'types' can be e.g. "quality", "performance", "energy consumption"
	 * 'measures' should be the "type of the measurment", therfore the unit of
	 * the value
	 * @param i: enumerates the names, measures and types. For the same value
	 * these should be related
	 */
	virtual std::string names(size_t i) = 0;
	virtual std::string types(size_t i) = 0;
	virtual std::string measures(size_t i) = 0;

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
			json.push_back({{"type", types(i)},
			                {"name", names(i)},
			                {"value", results[i]},
			                {"measures", measures(i)}});
		}
		return json;
	}
	/**
	 * Returns the name of the Benchmark. Again used for the output to the
	 * benchmark repository
	 */
	virtual std::string snab_name() = 0;

	virtual ~BenchmarkBase(){};
};
}

#endif /* SNABSUITE_COMMON_BENCHMARK_BASE_HPP */
