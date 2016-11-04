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

#ifndef SNABSUITE_SNABS_OUTPUT_BENCH_HPP
#define SNABSUITE_SNABS_OUTPUT_BENCH_HPP

#include <cypress/cypress.hpp>

#include "common/benchmark_base.hpp"

namespace SNAB {
class OutputFrequencySingleNeuron : public BenchmarkBase {
private:
	std::string m_backend;
	cypress::PopulationBase m_pop;
	cypress::Json m_config_file;
	const std::vector<std::string> names_vec = {
	    "Average frequency", "Standard deviation", "Maximum", "Minimum"};
	const std::vector<std::string> types_vec = {"quality", "quality", "quality",
	                                        "quality"};
	const std::vector<std::string> measures_vec = {"1/ms", "1/ms", "1/ms", "1/ms"};

public:
	OutputFrequencySingleNeuron(const std::string backend);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(std::string backend, cypress::Network &netw) override;
	std::vector<cypress::Real> evaluate() override;
	std::string names(size_t i) override { return names_vec[i]; };
	std::string types(size_t i) override { return types_vec[i]; };
	std::string measures(size_t i) override { return measures_vec[i]; };
    std::string snab_name(){return "OutputFrequencySingleNeuron";};
};
}
#endif /* SNABSUITE_SNABS_OUTPUT_BENCH_HPP */
