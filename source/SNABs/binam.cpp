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

#include "binam.hpp"
#include "core/parameters.hpp"
#include "core/spiking_binam.hpp"
#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;

BiNAM::BiNAM(const std::string backend, size_t bench_index)
    : BiNAM(backend, bench_index, __func__)
{
}

BiNAM::BiNAM(const std::string backend, size_t bench_index,
                       std::string name)
    : SNABBase(name, backend,
               {"Stored Information", "Relative Information", "False Postives",
                "False Negatives"},
               {"quality", "qualtiv", "quality", "quality"},
               {"bits?", "", "", ""}, {"bits?", "", "", ""},
               {"network", "data", "data_generator"},
               bench_index)  // TODO
{
}
cypress::Network &BiNAM::build_netw(cypress::Network &netw)
{
	m_sp_binam = std::make_shared<nam::SpikingBinam>(m_config_file, std::cout,
	                                                 false);
	m_sp_binam->build(netw);
	return netw;
}

void BiNAM::run_netw(cypress::Network &netw)
{
	std::thread spiking_network([&]() mutable {
		cypress::PowerManagementBackend pwbackend(
		    cypress::Network::make_backend(m_backend));
		netw.logger().min_level(cypress::LogSeverity::DEBUG);
		netw.run(pwbackend);
	});
	std::thread recall([&]() mutable { m_sp_binam->recall(); });
	recall.join();
	spiking_network.join();
}

std::vector<std::array<cypress::Real, 4>> BiNAM::evaluate()
{
	auto res = m_sp_binam->evaluate_res();
	return {
	    std::array<Real, 4>({Real(std::get<1>(res).Info), NaN(), NaN(), NaN()}),
	    std::array<Real, 4>({std::get<1>(res).Info / std::get<0>(res).Info,
	                         NaN(), NaN(), NaN()}),
	    std::array<Real, 4>({std::get<1>(res).fp, NaN(), NaN(), NaN()}),
	    std::array<Real, 4>({std::get<1>(res).fn, NaN(), NaN(), NaN()})};
}

}  // namespace SNAB
