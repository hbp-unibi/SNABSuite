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

#include <cypress/cypress.hpp>

#include <glob.h>
#include <csignal>

#include "common/parameter_sweep.hpp"

using namespace SNAB;

// Compatibility hack for for older glibc
__asm__(".symver glob64,glob64@GLIBC_2.2.5");

// Global Pointer to a sweep instance, used to access member in sig handler
ParameterSweep *sweep_pointer;

/**
 * Using the global pointer this wrapper executes the backup function when the
 * program gets a signal @param i
 */
void backup_wrapper_sig_handler(int i)
{
	std::cout << "Caught signal " << i << std::endl;
	sweep_pointer->backup_simulation_results();
	std::cout << "Backup complete!" << std::endl;
	std::abort();
}

int main(int argc, const char *argv[])
{
	if ((argc < 4 || argc > 6) && !cypress::NMPI::check_args(argc, argv)) {
		std::cout
		    << "Usage: " << argv[0]
		    << " <SIMULATOR> <SWEEP_CONFIG> <bench_index> [threads] [NMPI]"
		    << std::endl;
		return 1;
	}

	if (std::string(argv[argc - 1]) == "NMPI" &&
	    !cypress::NMPI::check_args(argc, argv)) {

		// Gather all config file to upload them to the server
		glob_t glob_result;
		glob(std::string("../config/*").c_str(), GLOB_TILDE, NULL,
		     &glob_result);
		std::vector<std::string> files;
		for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
			files.push_back(std::string(glob_result.gl_pathv[i]));
		}
		globfree(&glob_result);

		// Execute NMPI
		cypress::NMPI(argv[1], argc, argv, files);
		return 0;
	}

	size_t bench_index = std::stoi(argv[3]);

	size_t threads = 1;
	if ((argc > 4) && isdigit(*argv[argc - 1])) {
		threads = std::stoi(argv[argc - 1]);
	}

	// Open sweep config
	std::ifstream ifs(argv[2]);
	if (!ifs.good()) {
		std::cout << "Could not open sweep configuration file!" << std::endl;
		return 1;
	}
	auto json = cypress::Json::parse(ifs);

	// Suppress all logging
	cypress::global_logger().min_level(cypress::LogSeverity::ERROR, 1);

	ParameterSweep sweep(argv[1], json, bench_index, threads);

	// Use customized signal handler to backup sweep
	sweep_pointer = &sweep;
	std::signal(SIGINT, backup_wrapper_sig_handler);

	// Execute and evaluate
	try {
		sweep.execute();
	}
	catch (cypress::CypressException &e) {
		sweep_pointer->backup_simulation_results();
		std::cout << "Backup complete!" << std::endl;
		throw e;
	}
	sweep.write_csv();

	return 0;
}
