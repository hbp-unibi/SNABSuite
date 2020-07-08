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

#ifndef SNABSUITE_SNABS_BINAM_HPP
#define SNABSUITE_SNABS_BINAM_HPP

#include <cypress/cypress.hpp>
//#include <memory>

#include "common/snab_base.hpp"
#include "core/spiking_binam.hpp"

namespace SNAB {
/**
 * TODO
 */
class BiNAM : public SNABBase {
protected:
	std::shared_ptr<nam::SpikingBinam> m_sp_binam;

	/**
	 * @brief Constructor used by deriving classes
	 *
	 * @param backend backend
	 * @param bench_index benchmark index
	 * @param name Name of the derived benchmark
	 */
	BiNAM(const std::string backend, size_t bench_index, std::string name);

public:
	BiNAM(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<BiNAM>(m_backend, m_bench_index);
	}
};

class BiNAM_pop : public BiNAM {
public:
	BiNAM_pop(const std::string backend, size_t bench_index)
	    : BiNAM(backend, bench_index, __func__)
	{
	}
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<BiNAM_pop>(m_backend, m_bench_index);
	}
};
class BiNAM_burst : public BiNAM {
public:
	BiNAM_burst(const std::string backend, size_t bench_index)
	    : BiNAM(backend, bench_index, __func__)
	{
	}
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<BiNAM_burst>(m_backend, m_bench_index);
	}
};
class BiNAM_pop_burst : public BiNAM {
public:
	BiNAM_pop_burst(const std::string backend, size_t bench_index)
	    : BiNAM(backend, bench_index, __func__)
	{
	}
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<BiNAM_pop_burst>(m_backend, m_bench_index);
	}
};

}  // namespace SNAB

#endif
