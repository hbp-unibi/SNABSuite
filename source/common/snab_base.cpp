/*
 *  SNABSuite - Spiking Neural Architecture Benchmark Suite
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

#include <cypress/cypress.hpp>

#include "snab_base.hpp"

#include <sys/stat.h>
#include <string>

#include "util/read_json.hpp"
#include "util/utilities.hpp"

namespace SNAB {
SNABBase::SNABBase(std::string name, std::string backend,
                   std::initializer_list<std::string> indicator_names,
                   std::initializer_list<std::string> indicator_types,
                   std::initializer_list<std::string> indicator_measures,
                   std::initializer_list<std::string> required_parameters,
                   size_t bench_index)
    : m_backend(backend),
      m_snab_name(name),
      m_indicator_names(indicator_names),
      m_indicator_types(indicator_types),
      m_indicator_measures(indicator_measures)
{
	m_config_file = read_config(name, m_backend);
	std::vector<std::string> required_parameters_vec(required_parameters);
	bool required_params =
	    check_json_for_parameters(required_parameters_vec, m_config_file, name);

	// Check wether benchmark is labeled as invalid
	if ((m_config_file.find("invalid") == m_config_file.end() ||
	     bool(m_config_file["invalid"]) == false) &&
	    required_params) {
		m_valid = true;
	}
	else {
		return;
	}
	
	bool changed = replace_arrays_by_value(m_config_file, bench_index, name);
	if (!changed && bench_index != 0) {
		m_valid = false;
	}

	// Check for backend related setup config
	if (m_config_file.find("setup") != m_config_file.end()) {
		Utilities::manipulate_backend_string(m_backend, m_config_file["setup"]);
		m_config_file.erase("setup");
	}

	
}

cypress::Json SNABBase::evaluate_json()
{
	std::vector<cypress::Real> results = evaluate();
	cypress::Json json;
	for (size_t i = 0; i < results.size(); i++) {
		json.push_back({{"type", m_indicator_types[i]},
		                {"name", m_indicator_names[i]},
		                {"value", results[i]},
		                {"measures", m_indicator_measures[i]}});
	}
	return json;
}

std::string SNABBase::_debug_filename(const std::string append) const
{
	std::string shortened_backend =
	    Utilities::split(Utilities::split(m_backend, '=')[0], '.').back();
	mkdir("debug/", S_IRWXU | S_IRWXG);
	mkdir(("debug/" + shortened_backend + "/").c_str(), S_IRWXU | S_IRWXG);
	return std::string("debug/" + shortened_backend + "/" + m_snab_name + "_" +
	                   append);
}
cypress::Real NaN() { return std::numeric_limits<cypress::Real>::quiet_NaN(); }
}  // namespace SNAB
