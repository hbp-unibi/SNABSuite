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

#ifndef SNABSUITE_SNABS_FUNCTION_HPP
#define SNABSUITE_SNABS_FUNCTION_HPP

#include <cmath>
#include <cypress/cypress.hpp>
#include <cypress/nef.hpp>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * In this benchmark we approximate a function f(x) using a spiking population.
 * X in[0,1] is encoded linearly into a spike frequency, the response is
 * measured. Based on a first evaluation, we have neuron responses for n neurons
 * and m different x values. The resulting matrix is inverted to calculate
 * coefficients for every neuron to approximate the function. A second run looks
 * at the interpolation of the SNN and evaluates deviations from the target
 * function.
 */
class FunctionApproximation : public SNABBase {
protected:
	cypress::NeuronParameter m_neuro_params;
	cypress::nef::TuningCurveEvaluator m_evaluator_train, m_evaluator_test;
	cypress::Network m_netw_train, m_netw_test;

public:
	FunctionApproximation(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<FunctionApproximation>(m_backend,
		                                               m_bench_index);
	}
};
}  // namespace SNAB
#endif
