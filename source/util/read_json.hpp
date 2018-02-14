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
#include <limits>
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
                          const std::vector<std::string> &names,
                          const std::vector<T> &defaults)
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

cypress::Json read_config(const std::string &name, const std::string &backend);

bool check_json_for_parameters(const std::vector<std::string> &parameters,
                               const cypress::Json &json,
                               const std::string name);

/**
 * Extract backend specific part from a configuration file. Checks for dot
 * seperated backends
 */
cypress::Json extract_backend(const cypress::Json &config,
                              const std::string &backend);

/**
 * Function to convert a JSON array to a vector. This is used for one
 * dimensional arrays only!
 * @param json array to be converted. This shouold not be an object!
 * @return Vector of type T containing values of the array
 */
template <typename T>
std::vector<T> json_array_to_vector(const cypress::Json &json)
{
	if (!json.is_array() || json.size() == 0 || json[0].is_array()) {
		throw std::invalid_argument("Error in conversion from Json to array!");
	}
	std::vector<T> res;
	for (auto i : json) {
		if (i.is_null()) {
			res.push_back(std::numeric_limits<T>::quiet_NaN());
		}
		else {
			res.push_back(T(i));
		}
	}
	return res;
}

/**
 * Function to convert a two dimensional array to a vector.
 * @param json 2D array to be converted. This shouold not be an object!
 * @return Vector of vectors of type T containing values of the 2D array
 */
template <typename T>
std::vector<std::vector<T>> json_2Darray_to_vector(const cypress::Json &json)
{
	if (!json.is_array() || !json[0].is_array()) {
		throw std::invalid_argument("Error in conversion from Json to array!");
	}
	std::vector<std::vector<T>> res;
	for (auto i : json) {
		res.push_back(json_array_to_vector<T>(i));
	}
	return res;
}

/**
 * Replaces all arrays in a json object with one entry of the same array
 * @param json object to be manipulated
 * @param index index of value used
 * @param warn When set to true, warning is given when nothing has been replaced
 * @return true if json was changed
 */
bool replace_arrays_by_value(cypress::Json &json, const size_t &index = 0,
                             std::string name = "", bool warn = true);
}  // namespace SNAB

#endif /* SNABSUITE_UTIL_READ_JSON_HPP */
