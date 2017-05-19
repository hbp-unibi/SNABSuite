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

namespace SNAB {

/**
 * class for systematic parameter sweeps of single benchmarks
 */
class ParameterSweep {
private:
	// String containing the simulation backend
	std::string m_backend;
	// Sweep Configuration
	cypress::Json m_sweep_config;
	// Benchmark class
	std::shared_ptr<SNABBase> m_snab;
	// Shuffled indices
	std::vector<size_t> m_indices;
	// List of indices with jobs already done
	std::vector<size_t> m_jobs_done;
	// Vector containing all configuration files of a sweep
	std::vector<cypress::Json> m_sweep_vector;
	// Names and Keys for parameterws swept over
	std::vector<std::string> m_sweep_names;
	// Vector containing all resulting json files
	std::vector<std::vector<cypress::Real>> m_results;
	// TODO: Number of repetitions for every simulation
	size_t m_repetitions = 1;

	// Function for shuffling indices, reduces covariance between neighbouring
	// simulations on analogue hardware
	void shuffle_sweep_indices(size_t size);

	// TODO
	void recover_broken_simulation();
	// TODO
	void backup_simulation_results();

public:
	/**
	 * Constructor choses the appropriate SNAB. sets the most general structures
	 * above and generate the vector containing configurations for all sweeps.
	 * Sweep indices are shuffeld
	 * @param backend: the cypress backend
	 * @param config: json structure which looks similar to the json-files in
	 * config/ thus containing backend specific instructions. This json
	 * structure should have arrays like [a,b,c] instead of a parameter value
	 * to sweep from a to b in c steps.
	 */
	ParameterSweep(std::string backend, cypress::Json &config);

	/**
	 * Execution of the sweep simulations. Results are stored in m_results
	 */
	void execute();

	/**
	 * Generates the m_sweep_vector.
	 * @param target contains the original config from the snab
	 * @param source should contain single values which will overwrite those
	 * from @target in all simulations and json values like [a,b,c] which will
	 * generate vectors of config files, in which the parameter is varied from a
	 * to b in c steps.
	 * @param sweep_names will contain the flattened keys for all parameters
	 * swept over
	 */
	static std::vector<cypress::Json> generate_sweep_vector(
	    const cypress::Json &source, const cypress::Json &target,
	    std::vector<std::string> &sweep_names);

	/**
	 * Results are converted to comma seperated values and written to
	 * *sweep_parameters*_*backend*_.csv
	 */
	void write_csv();
};
}

#endif