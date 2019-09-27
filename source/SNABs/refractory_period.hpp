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

#ifndef SNABSUITE_SNABS_REFRACTORY_PERIOD_HPP
#define SNABSUITE_SNABS_REFRACTORY_PERIOD_HPP

#include <cypress/cypress.hpp>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * This SNAB looks at the voltage trace of a single neuron to measure the length
 * of the refractory period. The result is the deviation from the set parameter:
 * Negative values correspond to too short periods, positive values indicate a
 * period which is too long.
 */
class RefractoryPeriod : public SNABBase {
private:
	cypress::PopulationBase m_pop;
	cypress::Real m_tolerance = 1.0;
	cypress::Population<cypress::SpikeSourceArray> m_pop_source;
	NeuronParameters m_neuro_params;

public:
	RefractoryPeriod(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<RefractoryPeriod>(m_backend, m_bench_index);
	}
};
}  // namespace SNAB

#endif /* SNABSUITE_SNABS_REFRACTORY_PERIOD_HPP*/
