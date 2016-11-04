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

namespace SNAB {
/* Virtual Base class for SNABs/Benchmarks.
 * All Benchmarks should have seperate building of networks, execution and an
 * evaluation tasks
 */
class BenchmarkBase {
protected:
	cypress::Network m_netw;

public:
	virtual ~BenchmarkBase(){};
	virtual cypress::Network &build_netw(cypress::Network &netw) = 0;
	cypress::Network &build() { return build_netw(m_netw); };
	virtual void run_netw(std::string backend, cypress::Network &netw) = 0;
	void run(std::string backend) { run_netw(backend, m_netw); };
	virtual std::vector<cypress::Real> evaluate() = 0;
	virtual std::string names(size_t i) = 0;
	virtual std::string types(size_t i) = 0;
	virtual std::string measures(size_t i) = 0;
    virtual std::string snab_name() = 0;
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
};
}

#endif /* SNABSUITE_COMMON_BENCHMARK_BASE_HPP */
