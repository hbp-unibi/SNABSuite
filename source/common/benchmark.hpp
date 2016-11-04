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

#include <string>

#include <cypress/cypress.hpp>
#include "common/benchmark_registry.hpp"

namespace SNAB {

class BenchmarkExec {
private:
	std::string m_backend;
	cypress::Json results;
    std::string convert_time(int value){
        if(value<10){
            return '0' + std::to_string(value);
        }
        return std::to_string(value);
    }
	std::string timestamp()
	{
		auto ltime = time(NULL);
		auto Tm = localtime(&ltime);
		return std::to_string(1900+Tm->tm_year) + "-" + convert_time(Tm->tm_mon) +
		       "-" + convert_time(Tm->tm_mday) + "T" +
		       convert_time(Tm->tm_hour) + ":" + convert_time(Tm->tm_min) +
		       ":" + convert_time(Tm->tm_sec);
	}

public:
	BenchmarkExec(std::string backend) : m_backend(backend)
	{
		auto snab_vec = benchmark_registry(m_backend);
		for (auto i : snab_vec) {
			i->build();
			i->run(m_backend);
			results.push_back({{"name", i->snab_name()},
			                   {"timestamp", timestamp()},
			                   {"results", i->evaluate_json()}});
		}
		std::cout <<results.dump(4)<<std::endl;
	};
};
}

#endif /* SNABSUITE_COMMON_BENCHMARK_HPP */
