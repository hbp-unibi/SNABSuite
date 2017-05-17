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

#pragma once

#ifndef SNABSUITE_UTIL_READ_JSON_HPP
#define SNABSUITE_UTIL_READ_JSON_HPP

#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <cypress/cypress.hpp>

//#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
/**
* Store input form json in a map to check everything!
*/
template <typename T>
std::map<std::string, T> json_to_map(const cypress::Json &obj)
{
	std::map<std::string, T> input;
	for (auto i = obj.begin(); i != obj.end(); i++) {
		const cypress::Json value = i.value();
		// Suppress entries like Neuron_Type and sub-lists
		if (value.is_number()) {
			input.emplace(i.key(), value);
		}
	}
	return input;
}

/**
 * Checks if all values given are needed, terminate if not.
 * If a needed value is not given, take default value and warn user
 */
template <typename T>
std::vector<T> read_check(std::map<std::string, T> &input,
                          std::vector<std::string> names,
                          std::vector<T> defaults)
{
	std::vector<T> res(names.size());
	for (size_t i = 0; i < res.size(); i++) {
		auto it = input.find(names[i]);
		if (it != input.end()) {
			res[i] = it->second;
			input.erase(it);
		}
		else {
			res[i] = defaults.at(i);
			global_logger().debug("SNABSuite",
			                      "For " + names[i] + " the default value " +
			                          std::to_string(defaults[i]) + " is used");
		}
	}
	for (auto elem : input) {
		throw std::invalid_argument("Unknown parameter \"" + elem.first + "\"");
	}

	return res;
}

cypress::Json read_config(std::string name, std::string backend);

bool check_json_for_parameters(std::vector<std::string> &parameters,
                               cypress::Json &json, std::string name);

/**
 * Extract backend specific part from a configuration file. Checks for dot
 * seperated backends
 */
cypress::Json extract_backend(cypress::Json &config, std::string backend);
}

#endif /* SNABSUITE_UTIL_READ_JSON_HPP */
