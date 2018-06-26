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

#include <glob.h>

#include <cypress/cypress.hpp>
#include "common/benchmark.hpp"

using namespace SNAB;

int main(int argc, const char *argv[])
{
	if ((argc < 2 || argc > 5) && !cypress::NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0]
		          << " <SIMULATOR> [snab] [bench_index] [NMPI]" << std::endl;
		return 1;
	}

	if (std::string(argv[argc - 1]) == "NMPI" &&
	    !cypress::NMPI::check_args(argc, argv)) {
		glob_t glob_result;
		glob(std::string("../config/*").c_str(), GLOB_TILDE, NULL,
		     &glob_result);
		std::vector<std::string> files;

		for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
			files.push_back(std::string(glob_result.gl_pathv[i]));
		}
		globfree(&glob_result);
		cypress::NMPI(argv[1], argc, argv, files, true);
		return 0;
	}

	size_t bench_index = 0;
	if (isdigit(*argv[argc - 1])) {
		bench_index = std::stoi(argv[argc - 1]);
	}

	cypress::global_logger().min_level(cypress::LogSeverity::DEBUG, 1);

	std::string snab_name = "all";
	if (argc > 2 && std::string(argv[2]) != "NMPI" && !isdigit(*argv[2])) {
		snab_name = std::string(argv[2]);
	}
	BenchmarkExec bench(std::string(argv[1]), snab_name, bench_index);

	return 0;
}
