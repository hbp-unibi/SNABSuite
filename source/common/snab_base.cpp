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

#include "snab_base.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <cypress/cypress.hpp>
#include <string>

#include "util/read_json.hpp"
#include "util/utilities.hpp"

namespace SNAB {
SNABBase::SNABBase(std::string name, std::string backend,
                   std::initializer_list<std::string> indicator_names,
                   std::initializer_list<std::string> indicator_types,
                   std::initializer_list<std::string> indicator_measures,
                   std::initializer_list<std::string> indicator_units,
                   std::initializer_list<std::string> required_parameters,
                   size_t bench_index)
    : m_backend(backend),
      m_snab_name(name),
      m_indicator_names(indicator_names),
      m_indicator_types(indicator_types),
      m_indicator_measures(indicator_measures),
      m_indicator_units(indicator_units),
      m_bench_index(bench_index)
{
	m_config_file = read_config(name, m_backend);

	bool changed =
	    replace_arrays_by_value(m_config_file, m_bench_index, m_snab_name);
	if (!changed && m_bench_index != 0) {
		m_valid = false;
		return;
	}
	check_config(required_parameters);
}

void SNABBase::check_config(std::vector<std::string> required_parameters_vec)
{
	// Check wether benchmark is labeled as invalid
	if (!((m_config_file.find("invalid") == m_config_file.end() ||
	       bool(m_config_file["invalid"]) == false))) {
		m_valid = false;
		return;
	}
	if (check_json_for_parameters(required_parameters_vec, m_config_file,
	                              m_snab_name)) {
		m_valid = true;
	}
	else {
		m_valid = false;
		return;
	}

	// Check for backend related setup config
	if (m_config_file.find("setup") != m_config_file.end()) {
		Utilities::manipulate_backend_string(m_backend, m_config_file["setup"]);
	}
}

cypress::Json SNABBase::evaluate_json()
{
	auto results = evaluate();
	cypress::Json json;
	for (size_t i = 0; i < results.size(); i++) {
		cypress::Json temp;
		temp["name"] = m_indicator_names[i];
		temp["type"] = m_indicator_types[i];
		temp["value"] = results[i][0];
		temp["measure"] = m_indicator_measures[i];
		if (m_indicator_units[i] != "") {
			temp["units"] = m_indicator_units[i];
		}
		if (!(results[i][1] != results[i][1])) {
			temp["std_dev"] = results[i][1];
		}
		if (!(results[i][2] != results[i][2])) {
			temp["min"] = results[i][2];
		}
		if (!(results[i][3] != results[i][3])) {
			temp["max"] = results[i][3];
		}
		json.push_back(temp);
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

void SNABBase::set_config(cypress::Json json)
{
	m_config_file = json;
	check_config();
}

void SNABBase::overwrite_backend_config(Json setup, bool delete_old)
{
	auto backend_vec = Utilities::split(m_backend, '=');
	if (!delete_old) {
		if (backend_vec.size() > 1) {
			Json old = Json::parse(backend_vec[1]);
			auto temp = Utilities::merge_json(old, setup);
			m_backend = backend_vec[0];
			Utilities::manipulate_backend_string(m_backend, temp);
			return;
		}
	}
	m_backend = backend_vec[0];
	Utilities::manipulate_backend_string(m_backend, setup);
}

Json SNABBase::get_backend_config()
{
	auto backend_vec = Utilities::split(m_backend, '=');

	if (backend_vec.size() > 1) {
		return Json::parse(backend_vec[1]);
	}
	return Json();
}

}  // namespace SNAB
