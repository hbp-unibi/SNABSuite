/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2019  Christoph Ostrau
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

#ifndef SNABSUITE_SNABS_MNIST_HPP
#define SNABSUITE_SNABS_MNIST_HPP

#include <cypress/cypress.hpp>

#include "common/neuron_parameters.hpp"
#include "common/snab_base.hpp"

namespace SNAB {
/**
 * TODO
 */
class SimpleMnist : public SNABBase {
public:
	SimpleMnist(const std::string backend, size_t bench_index);
	cypress::Network &build_netw(cypress::Network &netw) override;
	void run_netw(cypress::Network &netw) override;
	std::vector<std::array<cypress::Real, 4>> evaluate() override;
	std::shared_ptr<SNABBase> clone() override
	{
		return std::make_shared<SimpleMnist>(m_backend, m_bench_index);
	}
private:
	NeuronParameters m_neuro_params;
    std::string m_neuron_type_str;
    size_t m_images, m_batchsize;
    cypress::Real m_duration, m_max_freq, m_pause;
    bool m_poisson;
    Real m_max_weight;
    
    std::vector<std::pair<std::vector<std::vector<Real>>, std::vector<uint16_t>>> m_batch_data;
    
    void create_deep_network(const cypress::Json &data, cypress::Network &netw);
};

}  // namespace SNAB
#endif
