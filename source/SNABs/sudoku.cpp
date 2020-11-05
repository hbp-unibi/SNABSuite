/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2020  Christoph Ostrau
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
#include <cypress/backend/power/power.hpp>  // Control of power via netw
#include <cypress/cypress.hpp>              // Neural network frontend
#include <vector>

#include "sudoku.hpp"
#include "utils/spikingSudokuSolver.hpp"

namespace SNAB {
using namespace cypress;

namespace {
bool overwrite(Sudoku &sudoku)
{
	vector<vector<int>> blank = sudoku.getSudokuBlank();
	for (int row = 0; row < sudoku.getNumOfSquares(); row++) {
		for (int col = 0; col < sudoku.getNumOfSquares(); col++) {
			if (blank[row][col] != 0 &&
			    blank[row][col] != sudoku.getEntryInSquare(row, col)) {
				return true;
			}
		}
	}
	return false;
}
}  // namespace

SpikingSudoku::SpikingSudoku(const std::string backend, size_t bench_index,
                             std::string name)
    : SNABBase(name, backend, {"duration", "time_to_sol"},
               {"quality", "quality"}, {"time", "realtime"}, {"ms", "s"},
               {"neuron_params", "sudoku", "population", "noise", "trigger",
                "start", "duration", "bin_size"},
               bench_index)
{
}

SpikingSudoku::SpikingSudoku(const std::string backend, size_t bench_index)
    : SNABBase(__func__, backend, {"duration", "time_to_sol"},
               {"quality", "quality"}, {"time", "realtime"}, {"ms", "s"},
               {"neuron_params", "sudoku", "population", "noise", "trigger",
                "start", "duration", "bin_size"},
               bench_index)
{
}

Network &SpikingSudoku::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	build_solver<spikingSudokuSolver>(netw);
	return netw;
}
void SpikingSudoku::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend);
}

std::vector<std::array<cypress::Real, 4>> SpikingSudoku::evaluate()
{
	m_solver->run("", "", false, false, true);
	auto result = m_solver->evaluate();
	Real duration = NaN();
	Sudoku sudoku;
	int bin_size = m_solver->config()["bin_size"];
	for (size_t k = 0; k < result[0][0].size() - 1; k++) {
		sudoku = m_solver->updateSudokuFromResult(*m_sudoku, result, k);
		if (sudoku.complete() && !overwrite(sudoku)) {
			duration = (Real)(k * bin_size);
			break;
		}
	}
	Real time_to_sol = NaN();
	if (duration == duration) {
		time_to_sol =
		    (duration / m_solver->duration()) * m_netw.runtime().sim_pure;
	}

	return {std::array<Real, 4>({duration, NaN(), NaN(), NaN()}),
	        std::array<Real, 4>({time_to_sol, NaN(), NaN(), NaN()})};
}

Network &SpikingSudokuSinglePop::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	build_solver<SpikingSolverSinglePop>(netw);
	return netw;
}

Network &SpikingSudokuMirrorInhib::build_netw(cypress::Network &netw)
{
	RNG::instance().seed(1234);
	build_solver<SSolveMirrorInhib>(netw);
	return netw;
}

}  // namespace SNAB
