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

#include <cypress/cypress.hpp>
#include "util/read_json.hpp"

#include "util/utilities.hpp"

namespace SNAB {
using cypress::global_logger;
cypress::Json read_config(std::string name, std::string backend)
{
	std::vector<std::string> dir(
	    {"../config/", "../../config/", "config/", ""});
	bool valid = false;
	cypress::Json config;

	for (auto i : dir) {
		std::ifstream ifs(i + name + ".json");
		if (ifs.good()) {
			config << ifs;
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

cypress::Json extract_backend(cypress::Json &config, std::string backend)
{
	if (config.find(backend) == config.end()) {
		std::string simulator =
		    Utilities::split(Utilities::split(backend, '=')[0], '.').back();
		if (config.find(simulator) == config.end()) {
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

bool check_json_for_parameters(std::vector<std::string> &parameters,
                               cypress::Json &json, std::string name)
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
}
