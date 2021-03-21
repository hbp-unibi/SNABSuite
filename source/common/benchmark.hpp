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

#include <cypress/cypress.hpp>
#include <fstream>
#include <string>

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

	/**
	 * Convert bench index to task string
	 */
	std::vector<std::string> bench_index_str{"Single Core/Smallest Network",
	                                         "Single Chip", "Small System",
	                                         "Large System"};

	/**
	 * Check if benchmark should be repeated + averaged
	 */
	size_t check_for_repeat(const cypress::Json &config)
	{
		if (config.count("repeat") > 0) {
			size_t rep = config["repeat"].get<size_t>();
			if (rep) {
				// Zero = do not repeat = 1
				return rep;
			}
		}
		return 1;
	}

	/**
	 * When repeating, merge results from several runs
	 */
	cypress::Json merge_repeat_results(
	    const std::vector<cypress::Json> &results);

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
		for (auto &i : snab_vec) {
			if (i->valid() &&
			    (benchmark == "all" || benchmark == i->snab_name())) {
				global_logger().info("SNABSuite",
				                     "Executing " + i->snab_name());

				size_t repeat = check_for_repeat(i->get_config());
				if (repeat == 1) {
					i->build();
					i->run();
					results.push_back({{"model", i->snab_name()},
					                   {"timestamp", timestamp()},
					                   {"task", bench_index_str[bench_index]},
					                   {"results", i->evaluate_json()}});
					// Clear memory
					i.reset();
				}
				else {
					std::vector<cypress::Json> repeat_results;
					i->build();
					for (size_t j = 0; j < repeat; j++) {
						i->run();
						repeat_results.push_back(i->evaluate_json());
					}
					results.push_back(
					    {{"model", i->snab_name()},
					     {"timestamp", timestamp()},
					     {"task", bench_index_str[bench_index]},
					     {"results", merge_repeat_results(repeat_results)}});
					// Clear memory
					i.reset();
				}
			}
		}
		std::cout << results.dump(4) << std::endl;
		{
			std::fstream file;
			file.open(
			    (backend + "_" + std::to_string(bench_index) + ".json").c_str(),
			    std::fstream::out);
			if (results.size() > 1) {
				file << results.dump(4) << std::endl;
			}
			else {
				file << results[0].dump(4) << std::endl;
			}
			file.close();
		}
	};
};
}  // namespace SNAB

#endif /* SNABSUITE_COMMON_BENCHMARK_HPP */
