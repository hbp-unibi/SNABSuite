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

#ifndef SNABSUITE_COMMON_SPIKING_PARAMETERS_HPP
#define SNABSUITE_COMMON_SPIKING_PARAMETERS_HPP

#include <cypress/cypress.hpp>

#include <array>
#include <iostream>
#include <string>

/**
 * Macro used for defining the getters and setters associated with a parameter
 * value.
 */
#define NAMED_PARAMETER(NAME, IDX)            \
	static constexpr size_t idx_##NAME = IDX; \
	void NAME(Real x) { arr[IDX] = x; }       \
	Real &NAME() { return arr[IDX]; }         \
	Real NAME() const { return arr[IDX]; }

namespace SNAB {
using Real = cypress::Real;

class NeuronParameters {
private:
	std::vector<cypress::Real> m_params;
	std::vector<std::string> m_parameter_names;

public:
	/**
	 * Construct from Json, logging values to DEBUG
	 */
	NeuronParameters(const cypress::NeuronType &type,
	                 const cypress::Json &json);

	NeuronParameters(){};

	const std::vector<cypress::Real> &parameter() const { return m_params; };

	/**
	 * Set parameter with name @param name to @param value
	 */
	NeuronParameters &set(std::string name, cypress::Real value)
	{
		for (size_t i = 0; i < m_parameter_names.size(); i++) {
			if (m_parameter_names[i] == name) {
				m_params[i] = value;
				return *this;
			}
		}
		throw std::invalid_argument("Unknown neuron parameter" + name);
	}

	cypress::Real get(std::string name)
	{
		for (size_t i = 0; i < m_parameter_names.size(); i++) {
			if (m_parameter_names[i] == name) {
				return m_params[i];
			}
		}
		throw std::invalid_argument("Unknown neuron parameter" + name);
	}
	void print(std::ostream &out = std::cout)
	{
		out << "# Neuron Parameters: " << std::endl;
		for (size_t i = 0; i < m_params.size(); i++) {
			out << m_parameter_names[i] << ": " << m_params[i] << std::endl;
		}
	}
};
}
#undef NAMED_PARAMETER
#endif /* SNABSUITE_COMMON_SPIKING_PARAMETERS_HPP */
