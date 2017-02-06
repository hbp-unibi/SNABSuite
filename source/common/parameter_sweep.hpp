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

#ifndef SNABSUITE_COMMON_PARAMETER_SWEEP
#define SNABSUITE_COMMON_PARAMETER_SWEEP

#include <fstream>
#include <memory>
#include <string>

#include <cypress/cypress.hpp>
#include "common/snab_base.hpp"
#include "common/snab_registry.hpp"

namespace SNAB {
class ParameterSweep {
private:
	// String containing the simulation backend
	std::string m_backend;
	// Container for the results
	cypress::Json results;
	// Sweep Configuration
	cypress::Json m_sweep_config;
	// Benchmark class
	std::shared_ptr<SNABBase> snab;

public:
	ParameterSweep(std::string backend, cypress::Json &config)
	    : m_backend(backend), m_sweep_config(config)
	{
		std::string snab_name = m_sweep_config["snab_name"];
		auto snab_vec = snab_registry(m_backend);
		for (auto i : snab_vec) {
			if (i->snab_name == snab_name) {
				snab = i;
			}
		}
	}

	void execute() {}

	static std::vector<cypress::Json> generate_sweep_vector(
	    cypress::Json &source, cypress::Json &target,
	    std::vector<std::string> &sweep_values);
    
    void prepare_json();
};
}

#endif
