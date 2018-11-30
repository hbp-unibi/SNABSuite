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

#include <cypress/cypress.hpp>  // Neural network frontend

#include <cypress/backend/power/netio4.hpp>  // Control of power via NetIO4 Bank

#include <util/spiking_utils.hpp>
#include "setup_time.hpp"

namespace SNAB {

SetupTimeOneToOne::SetupTimeOneToOne(const std::string backend,
                                     size_t bench_index)
    : SNABBase(__func__, backend, {"Setup Time", "Speedup"},
               {"performance", "quality"}, {"time", "speedup"}, {"ms", ""},
               {"#neurons", "neuron_type"}, bench_index),
      m_pop1(m_netw, 0),
      m_pop2(m_netw, 0)
{
}

cypress::Network &SetupTimeOneToOne::build_netw(cypress::Network &netw)
{
	std::string neuron_type_str = m_config_file["neuron_type"];
	NeuronParameters params = NeuronParameters(
	    SpikingUtils::detect_type(neuron_type_str), cypress::Json());
	m_pop1 = SpikingUtils::add_population(neuron_type_str, netw, params,
	                                      m_config_file["#neurons"]);
	m_pop2 = SpikingUtils::add_population(neuron_type_str, netw, params,
	                                      m_config_file["#neurons"]);
	SpikingUtils::add_population(neuron_type_str, m_netw2, params, 1);
	return netw;
}

void SetupTimeOneToOne::run_netw(cypress::Network &netw)
{
	netw.add_connection(m_pop1, m_pop2, Connector::one_to_one(1.0, 1.0));
	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	m_netw2.run(pwbackend, 1);
	netw.run(pwbackend, 1);
	m_rt_conn = netw.runtime().sim;

	netw = cypress::Network();
	build_netw(netw);
	netw.add_connection(
	    m_pop1, m_pop2,
	    Connector::fixed_probability(Connector::one_to_one(1.0, 1.0), 1.0));
	netw.run(pwbackend, 1);
	m_rt_list = netw.runtime().sim;
}

std::vector<std::array<cypress::Real, 4>> SetupTimeOneToOne::evaluate()
{
	return {std::array<cypress::Real, 4>({m_rt_conn, NaN(), NaN(), NaN()}),
	        std::array<cypress::Real, 4>(
	            {m_rt_list / m_rt_conn, NaN(), NaN(), NaN()})};
}

SetupTimeOneToOne::SetupTimeOneToOne(
    std::string name, std::string backend,
    std::initializer_list<std::string> indicator_names,
    std::initializer_list<std::string> indicator_types,
    std::initializer_list<std::string> indicator_measures,
    std::initializer_list<std::string> indicator_units,
    std::initializer_list<std::string> required_parameters, size_t bench_index)
    : SNABBase(name, backend, indicator_names, indicator_types,
               indicator_measures, indicator_units, required_parameters,
               bench_index),
      m_pop1(m_netw, 0),
      m_pop2(m_netw, 0)
{
}

SetupTimeAllToAll::SetupTimeAllToAll(const std::string backend,
                                     size_t bench_index)
    : SetupTimeOneToOne(__func__, backend, {"Setup Time", "Speedup"},
                        {"performance", "quality"}, {"time", "speedup"},
                        {"ms", ""}, {"#neurons", "neuron_type"}, bench_index)
{
}

void SetupTimeAllToAll::run_netw(cypress::Network &netw)
{
	netw.add_connection(m_pop1, m_pop2, Connector::all_to_all(1.0, 1.0));
	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	m_netw2.run(pwbackend, 1);
	netw.run(pwbackend, 1);
	m_rt_conn = netw.runtime().sim;

	netw = cypress::Network();
	build_netw(netw);
	netw.add_connection(
	    m_pop1, m_pop2,
	    Connector::fixed_probability(Connector::all_to_all(1.0, 1.0), 1.0));
	netw.run(pwbackend, 1);
	m_rt_list = netw.runtime().sim;
}

SetupTimeRandom::SetupTimeRandom(const std::string backend, size_t bench_index)
    : SetupTimeOneToOne(__func__, backend, {"Setup Time", "Speedup"},
                        {"performance", "quality"}, {"time", "speedup"},
                        {"ms", ""}, {"#neurons", "neuron_type"}, bench_index)
{
}

void SetupTimeRandom::run_netw(cypress::Network &netw)
{
	netw.add_connection(m_pop1, m_pop2, Connector::random(1.0, 1.0, 0.5));
	// PowerManagementBackend to use netio4
	cypress::PowerManagementBackend pwbackend(
	    std::make_shared<cypress::NetIO4>(),
	    cypress::Network::make_backend(m_backend));
	m_netw2.run(pwbackend, 1);
	netw.run(pwbackend, 1);
	m_rt_conn = netw.runtime().sim;

	netw = cypress::Network();
	build_netw(netw);
	netw.add_connection(
	    m_pop1, m_pop2,
	    Connector::fixed_probability(Connector::random(1.0, 1.0, 0.5), 1.0));
	netw.run(pwbackend, 1);
	m_rt_list = netw.runtime().sim;
}
}  // namespace SNAB
