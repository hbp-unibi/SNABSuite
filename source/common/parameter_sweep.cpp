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

#include <string>
#include <vector>

#include <cypress/cypress.hpp>
#include "parameter_sweep.hpp"

namespace SNAB {

namespace {
/**
 * Splits a string @param s into parts devided by @param delim and stores the
 * result in @param elems and returns it
 */
std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (item == "") {
			continue;
		}
		elems.push_back(item);
	}
	return elems;
}

/**
 * The same as above, but only returning the vector.
 */
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}
}

std::vector<cypress::Json> ParameterSweep::generate_sweep_vector(
    cypress::Json &source, cypress::Json &target,
    std::vector<std::string> &sweep_values)
{
	// vector containing the flatten JSON files
	std::vector<cypress::Json> sweep;
	// Flatting the input configs
	auto tar = target.flatten();
	auto src = source.flatten();

	// Copy single values, search for sweep entries
	for (auto i = src.begin(); i != src.end(); i++) {
		auto val = i.value();
		auto target_iter = tar.find(i.key());
		// If a value cannot be found in target, there are two possibilities:
		// Wrong entry or a sweep array was changed in entry/0, entry/1 and
		// entry/2.
		if (target_iter == tar.end()) {
			auto splitted = split(i.key(), '/');
			if (splitted.back() == "0") {
				std::string name;
				for (size_t i = 0; i < splitted.size() - 1; i++) {
					name.append("/" + splitted[i]);
				}
				sweep_values.push_back(name);
			}
			else if (splitted.back() == "1" || splitted.back() == "2") {
			}
			else {
				std::cerr << "Skipping value for " << i.key() << std::endl;
			}
		}
		// Copy single values
		else if (val.is_number() || val.is_string()) {
			if (i.key() == "repetitions") {
				continue;
			}
			tar[i.key()] = i.value();
		}
	}

	sweep.emplace_back(tar);
	for (auto i : sweep_values) {
		// Make a copy of the old vector
		std::vector<cypress::Json> sweep_temp = sweep;
		sweep = std::vector<cypress::Json>();
		// Gather relevant entries and calculate step size
		cypress::Real begin = src[i + "/0"];
		cypress::Real end = src[i + "/1"];
		cypress::Real steps = src[i + "/2"];
		cypress::Real step_size = (end - begin) / (steps - 1.0);
		for (size_t j = 0; j < steps; j++) {
			// For every Json in sweep_temp overwrite the respective value with
			// the current one
			cypress::Real current_value = begin + j * step_size;
			for (size_t k = 0; k < sweep_temp.size(); k++) {
				sweep_temp[k][i] = current_value;
			}
			// Insert this subpart (constant current_value) into the result
			sweep.insert(sweep.end(), sweep_temp.begin(), sweep_temp.end());
		}
	}

	// Unflatten the results
	std::vector<cypress::Json> sweep2;
	for (size_t i = 0; i < sweep.size(); i++) {
		sweep2.emplace_back(sweep[i].unflatten());
	}

	return sweep2;
}

void ParameterSweep::prepare_json()
{
	const std::vector<std::string> names = {"min", "max", "count"};
	size_t repetitions = 1;
	if (m_sweep_config.find("repetitions") != m_sweep_config.end()) {
		repetitions = m_sweep_config["repetitions"];
	}
}
}
