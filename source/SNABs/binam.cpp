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

BiNAM::BiNAM(const std::string backend, size_t bench_index, std::string name)
    : SNABBase(name, backend,
               {"Stored Information per sample", "Relative Information",
                "Normed False Positives", "Normed False Negatives"},
               {"quality", "qualtiv", "quality", "quality"},
               {"Information", "Information normed", "fp", "fn"},
               {"bits", "", "", ""}, {"network", "data", "data_generator"},
               bench_index)
{
}
cypress::Network &BiNAM::build_netw(cypress::Network &netw)
{
	if (m_config_file["data"].count("n_samples") == 0) {
		auto params = nam::DataParameters(m_config_file["data"]);
		m_config_file["data"]["n_samples"] = params.samples();
		global_logger().info(
		    "SNABSuite",
		    "Train BiNAM using " +
		        std::to_string(
		            m_config_file["data"]["n_samples"].get<size_t>()) +
		        " Samples.");
	}
#if SNAB_DEBUG
	m_sp_binam =
	    std::make_shared<nam::SpikingBinam>(m_config_file, std::cout, false);
#else
	std::ofstream ofs;
	ofs.open("/dev/null");
	m_sp_binam = std::make_shared<nam::SpikingBinam>(m_config_file, ofs, false);
#endif
	m_sp_binam->build(netw);
	return netw;
}

void BiNAM::run_netw(cypress::Network &netw)
{

	std::thread recall([&]() mutable { m_sp_binam->recall(); });
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend);
	recall.join();
}

std::vector<std::array<cypress::Real, 4>> BiNAM::evaluate()
{

#if SNAB_DEBUG
	auto pop = m_sp_binam->get_pop_output();
	std::vector<std::vector<cypress::Real>> spikes;
	for (size_t i = 0; i < pop.size(); i++) {
		spikes.push_back(pop[i].signals().data(0));
	}
	Utilities::write_vector2_to_csv(spikes, _debug_filename("spikes_out.csv"));
	Utilities::plot_spikes(_debug_filename("spikes_out.csv"), m_backend);

	std::ofstream ofs;
	ofs.open(_debug_filename("matrices.csv"));
	m_sp_binam->get_BiNAM()->print(ofs);
	ofs.close();
#endif

	auto rec_samples = Real(
	    m_config_file["data"]["n_samples"].get<size_t>());  // Samples to train
	if (m_config_file["network"].count("n_samples_recall") > 0) {
		size_t tmp = m_config_file["network"]["n_samples_recall"].get<size_t>();
		if (tmp > 0)
			rec_samples = tmp;
	}
	auto res = m_sp_binam->evaluate_res();
	Real d = m_config_file["data"]["n_ones_out"].get<size_t>();
	Real n = m_config_file["data"]["n_bits_out"].get<size_t>();
	Real norm_fp = 0, norm_fn = 0;
	if (std::get<1>(res).fp <= std::get<0>(res).fp) {
		norm_fp = (std::get<1>(res).fp / std::get<0>(res).fp) - 1.0;
	}
	else {
		norm_fp = (std::get<1>(res).fp - std::get<0>(res).fp) / rec_samples /
		          (n - d - (std::get<0>(res).fp / rec_samples));
	}
	norm_fn = std::get<1>(res).fn / rec_samples / d;

	return {std::array<Real, 4>({Real(std::get<1>(res).Info) / rec_samples,
	                             NaN(), NaN(), NaN()}),
	        std::array<Real, 4>({std::get<1>(res).Info / std::get<0>(res).Info,
	                             NaN(), NaN(), NaN()}),
	        std::array<Real, 4>({norm_fp, NaN(), NaN(), NaN()}),
	        std::array<Real, 4>({norm_fn, NaN(), NaN(), NaN()})};
}

}  // namespace SNAB
