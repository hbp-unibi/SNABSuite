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

#ifndef SNABSUITE_COMMON_BENCHMARK_HPP
#define SNABSUITE_COMMON_BENCHMARK_HPP

#include <fstream>
#include <string>

#include <cypress/cypress.hpp>
#include "common/snab_registry.hpp"

namespace SNAB {

using cypress::global_logger;
/**
 * Class for the consecutive execution of all benchmarks/SNABs registered in the
 * snab_registry.hpp
 */
class BenchmarkExec {
private:
	// String containing the simulation backend
	std::string m_backend;
	// Container for the results
	cypress::Json results;

	/**
	 * Converting an integer value to a time string
	 */
	std::string convert_time(int value)
	{
		if (value < 10) {
			return '0' + std::to_string(value);
		}
		return std::to_string(value);
	}

	/**
	 * Gives back a timestamp as a string without white spaces
	 */
	std::string timestamp()
	{
		auto ltime = time(NULL);
		auto Tm = localtime(&ltime);
		return std::to_string(1900 + Tm->tm_year) + "-" +
		       convert_time(Tm->tm_mon) + "-" + convert_time(Tm->tm_mday) +
		       "T" + convert_time(Tm->tm_hour) + ":" +
		       convert_time(Tm->tm_min) + ":" + convert_time(Tm->tm_sec);
	}

public:
	/**
	 * Constructor which executes all registered benchmarks and gives the result
	 * to std::cout and backend.json
	 */
	BenchmarkExec(std::string backend, std::string benchmark = "all",
	              size_t bench_index = 0)
	    : m_backend(backend)
	{
		auto snab_vec = snab_registry(m_backend, bench_index);
		for (auto i : snab_vec) {
			if (i->valid() &&
			    (benchmark == "all" || benchmark == i->snab_name)) {
				global_logger().info("SNABSuite", "Executing " + i->snab_name);
				i->build();
				i->run();
				results.push_back({{"name", i->snab_name},
				                   {"timestamp", timestamp()},
				                   {"results", i->evaluate_json()}});
			}
		}
		std::cout << results.dump(4) << std::endl;
		{
			std::fstream file;
			file.open(
			    (backend + "_" + std::to_string(bench_index) + ".json").c_str(),
			    std::fstream::out);
			file << results.dump(4) << std::endl;
			file.close();
		}
	};
};
}

#endif /* SNABSUITE_COMMON_BENCHMARK_HPP */
