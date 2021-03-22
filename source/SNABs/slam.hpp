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

#ifndef SNABSUITE_SNABS_SLAM_HPP
#define SNABSUITE_SNABS_SLAM_HPP

#include <cypress/cypress.hpp>
#include "spikingnetwork.h"
//#include <memory>

#include "common/snab_base.hpp"

namespace SNAB {
/**
 * Implements the mapping part of Simultaneous Localization and Mapping: a 2D
 * robot navigates in a virtual environment. A bumper sensor is triggered
 * whenever the robot touches a wall or obstacle. The internal map of the
 * surrounding is built using STDP.
 */
class SpikingSlam : public SNABBase {
private:
	// The STDP connection
	cypress::ConnectionDescriptor m_conn =
	    cypress::ConnectionDescriptor(0, 0, 0, 0, 0, 0);
	// The size of the map
	size_t xsize = 0, ysize = 0;
	std::vector<std::vector<bool>> m_map;  // The map itself
	cypress::Real m_scale_th = 0.25;        // Scale for the threshold in [0:1]
	std::shared_ptr<SpikingNetwork> m_slam;

public:
	SpikingSlam(const std::string backend, size_t bench_index);
	virtual cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	virtual std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SpikingSlam>(m_backend, m_bench_index);
	}
};
}  // namespace SNAB
#endif
