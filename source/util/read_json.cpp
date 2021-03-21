/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
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

#include "util/read_json.hpp"

#include <cypress/cypress.hpp>

#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
cypress::Json read_config(const std::string &name, const std::string &backend)
{
	std::vector<std::string> dir(
	    {"../config/", "../../config/", "config/", ""});
	bool valid = false;
	cypress::Json config;

	for (auto i : dir) {
		std::ifstream ifs(i + name + ".json");
		if (ifs.good()) {
			ifs >> config;
			valid = true;
			break;
		}
	}
	if (!valid) {
		global_logger().warn("SNABSuite",
		                     "Config file for " + name + " not found!");
		config["valid"] = false;
		return config;
	}

	if (config.find(backend) == config.end()) {
		std::string simulator =
		    Utilities::split(Utilities::split(backend, '=')[0], '.').back();
		if (config.find(simulator) == config.end()) {
			if (simulator == "genn_gpu") {
				simulator = "genn";
				if (config.find(simulator) != config.end()) {
					return config[simulator];
				}
			}
			global_logger().warn("SNABSuite",
			                     "Could not find any config for " + simulator +
			                         " in the config file of " + name + "! ");
			if (config.find("default") != config.end()) {
				global_logger().warn("SNABSuite",
				                     "Take default values instead!");
				return config["default"];
			}
			else {
				global_logger().warn(
				    "SNABSuite",
				    "Take values for " + config.begin().key() + " instead!");
				return config.begin().value();
			}
		}
		else {
			return config[simulator];
		}
	}
	else {
		return config[backend];
	}
}

cypress::Json extract_backend(const cypress::Json &config,
                              const std::string &backend)
{
	if (config.find(backend) == config.end()) {
		std::string simulator =
		    Utilities::split(Utilities::split(backend, '=')[0], '.').back();
		if (config.find(simulator) == config.end()) {
			if (simulator == "genn_gpu") {
				simulator = "genn";
				if (config.find(simulator) != config.end()) {
					return config[simulator];
				}
			}
			cypress::global_logger().warn(
			    "SNABSuite", "Could not find any config for " + simulator +
			                     " in the provided Json!");
			if (config.find("default") != config.end()) {
				global_logger().warn("SNABSuite",
				                     "Take default values instead!");
				return config["default"];
			}
			else {
				global_logger().warn(
				    "SNABSuite",
				    "Take values for " + config.begin().key() + " instead!");
				return config.begin().value();
			}
		}
		else {
			return config[simulator];
		}
	}
	else {
		return config[backend];
	}
}

bool check_json_for_parameters(const std::vector<std::string> &parameters,
                               const cypress::Json &json,
                               const std::string name)
{
	for (auto i : parameters) {
		if (json.find(i) == json.end()) {
			global_logger().warn("SNABSuite",
			                     "Config file for " + name +
			                         " does not contain any value for " + i);
			return false;
		}
	}
	return true;
}

bool replace_arrays_by_value(cypress::Json &json, const size_t &index,
                             std::string name, bool warn)
{
	bool anything_changed = false;
	for (Json::iterator i = json.begin(); i != json.end(); ++i) {
		if (i.value().is_object()) {
			bool temp = replace_arrays_by_value(i.value(), index, name, false);
			if (!anything_changed && temp) {
				anything_changed = true;
			}
		}
		else if (i.value().is_array()) {
			if (i.value().size() <= index) {
				global_logger().debug(
				    "SNABSuite", name + ": The array of size " +
				                     std::to_string(i.value().size()) +
				                     " is too small for requested index of " +
				                     std::to_string(index));
				return false;
			}
			i.value() = i.value()[index];
			anything_changed = true;
		}
	}
	if (!anything_changed && index != 0) {
		if (warn) {
			cypress::global_logger().debug(
			    "SNABSuite", name +
			                     ": Benchmark index is not zero, but no "
			                     "array was found in config file!");
		}
	}

	return anything_changed;
}
}  // namespace SNAB
