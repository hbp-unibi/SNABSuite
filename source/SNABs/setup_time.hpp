/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2018  Christoph Jenzen
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

#ifndef SNABSUITE_SNABS_SETUP_TIME_HPP
#define SNABSUITE_SNABS_SETUP_TIME_HPP

#include <cypress/cypress.hpp>

#include "common/snab_base.hpp"

namespace SNAB {

class SetupTimeOneToOne : public SNABBase {
protected:
	// Populations to be connected
	cypress::PopulationBase m_pop1, m_pop2;
	// Simulation times for Connector and List connection
	cypress::Real m_rt_conn, m_rt_list;
	// Dummy network to assure the machine is booted
	cypress::Network m_netw2;

	/**
	 * Constructor used by by child classes. Just hands everything to SNABBase
	 * class Constructor.
	 */
	SetupTimeOneToOne(std::string name, std::string backend,
	                  std::initializer_list<std::string> indicator_names,
	                  std::initializer_list<std::string> indicator_types,
	                  std::initializer_list<std::string> indicator_measures,
	                  std::initializer_list<std::string> indicator_units,
	                  std::initializer_list<std::string> required_parameters,
	                  size_t bench_index);

public:
	SetupTimeOneToOne(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
};

class SetupTimeAllToAll : public SetupTimeOneToOne {
public:
	SetupTimeAllToAll(const std::string backend, size_t bench_index);
	void run_netw(cypress::Network &netw) override;
};

class SetupTimeRandom : public SetupTimeOneToOne {
public:
	SetupTimeRandom(const std::string backend, size_t bench_index);
	void run_netw(cypress::Network &netw) override;
};
}  // namespace SNAB

#endif /* SNABSUITE_SNABS_SETUP_TIME_HPP */
