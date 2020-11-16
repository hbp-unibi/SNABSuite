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

#pragma once

#ifndef SNABSUITE_SNABS_SUDOKU_HPP
#define SNABSUITE_SNABS_SUDOKU_HPP

#include <cypress/cypress.hpp>
//#include <memory>

#include "common/snab_base.hpp"
#include "source/utils/Sudoku.hpp"
#include "utils/spikingSudokuSolver.hpp"

namespace SNAB {
/**
 * Simplest, straight forward method to solve a Sudoku with SNNs. Solves a
 * single SNN using a WTA network.
 */
class SpikingSudoku : public SNABBase {
protected:
	// size_t m_height = 0, m_width = 0; // ?
	std::shared_ptr<spikingSudokuSolver> m_solver;
	std::shared_ptr<Sudoku> m_sudoku;

	template <typename T>
	void build_solver(cypress::Network netw)
	{
		m_solver = std::make_shared<T>(m_config_file);
		m_sudoku = std::make_shared<Sudoku>(
		    m_config_file["sudoku"].get<std::string>());
		m_solver->initialize(*m_sudoku, netw);
	}
	SpikingSudoku(const std::string backend, size_t bench_index,
	              std::string name);

public:
	SpikingSudoku(const std::string backend, size_t bench_index);
	virtual cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SpikingSudoku>(m_backend, m_bench_index);
	}
};

/**
 * @brief Identical to SpikingSudoku. However, the network structure is defined
 * in a different way, using only a single population and two connectors. Meant
 * to improve speed on spinnaker + GeNN.
 *
 */
class SpikingSudokuSinglePop : public SpikingSudoku {
public:
	SpikingSudokuSinglePop(const std::string backend, size_t bench_index)
	    : SpikingSudoku(backend, bench_index, __func__)
	{
	}
	cypress::Network &build_netw(cypress::Network &netw) override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SpikingSudokuSinglePop>(m_backend,
		                                                m_bench_index);
	}
};

/**
 * @brief Similar to SpikingSudoku, but using mirror populations to implement
 * the inhibitory part of the network. This is specifically designed for spikey,
 * which does not allow to have outgoing inh/exc connections at the same time.
 */
class SpikingSudokuMirrorInhib : public SpikingSudoku {
public:
	SpikingSudokuMirrorInhib(const std::string backend, size_t bench_index)
	    : SpikingSudoku(backend, bench_index, __func__)
	{
	}
	cypress::Network &build_netw(cypress::Network &netw) override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SpikingSudokuMirrorInhib>(m_backend,
		                                                  m_bench_index);
	}
};

}  // namespace SNAB
#endif
