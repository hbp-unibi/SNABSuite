/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2016  Christoph Jenzen, Andreas Stöckel
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

#include <algorithm>

#include <cypress/cypress.hpp>

#include "spiking_utils.hpp"

namespace SNAB {
using namespace cypress;
const NeuronType &SpikingUtils::detect_type(std::string neuron_type_str)
{
	if (neuron_type_str == "IF_cond_exp") {
		return IfCondExp::inst();
	}
	else if (neuron_type_str == "IfFacetsHardware1") {
		return IfFacetsHardware1::inst();
	}
	else if (neuron_type_str == "AdExp") {
		return EifCondExpIsfaIsta::inst();
	}
	throw CypressException("Invalid neuron type \"" + neuron_type_str + "\"");
}

template <typename T>
PopulationBase SpikingUtils::add_typed_population(
    Network &network, const NeuronParameters &neuronParams, const size_t size,
    const typename T::Signals &rec_signal)
{
	using Parameters = typename T::Parameters;
	return network.create_population<T>(
	    size, Parameters(neuronParams.parameter()), rec_signal);
}

template <typename T>
PopulationBase SpikingUtils::add_typed_population_no_record(
    Network &network, const NeuronParameters &neuronParams, const size_t size)
{
	using Parameters = typename T::Parameters;
	return network.create_population<T>(size,
	                                    Parameters(neuronParams.parameter()));
}

PopulationBase SpikingUtils::add_population(
    const std::string neuron_type_str, Network &network,
    const NeuronParameters &neuronParams, const size_t size,
    const std::string record_signal)
{
	if (record_signal == "") {
		if (neuron_type_str == "IF_cond_exp") {
			return add_typed_population_no_record<IfCondExp>(
			    network, neuronParams, size);
		}
		else if (neuron_type_str == "IfFacetsHardware1") {
			return add_typed_population_no_record<IfFacetsHardware1>(
			    network, neuronParams, size);
		}
		else if (neuron_type_str == "AdExp") {
			return add_typed_population_no_record<EifCondExpIsfaIsta>(
			    network, neuronParams, size);
		}
	}
	if (neuron_type_str == "IF_cond_exp") {
		return add_typed_population<IfCondExp>(
		    network, neuronParams, size, IfCondExpSignals({record_signal}));
	}
	else if (neuron_type_str == "IfFacetsHardware1") {
		return add_typed_population<IfFacetsHardware1>(
		    network, neuronParams, size,
		    IfFacetsHardware1Signals({record_signal}));
	}
	else if (neuron_type_str == "AdExp") {
		return add_typed_population<EifCondExpIsfaIsta>(
		    network, neuronParams, size,
		    EifCondExpIsfaIstaSignals({record_signal}));
	}

	throw CypressException("Invalid neuron type \"" + neuron_type_str + "\"");
}

bool SpikingUtils::rerun_fixed_number_trials(Network &network, Backend &backend,
                                             Real time, size_t n_trials)
{
	for (size_t i = 0; i < n_trials; i++) {
		try {
			network.run(backend, time);
			return true;
		}
		catch (const std::exception &exc) {
			std::cerr << exc.what();
			global_logger().fatal_error("SNABSuite",
			                            "Wrong parameter setting or backend "
			                            "error! Simulation broke down");
		}
	}
	return false;
}
}
