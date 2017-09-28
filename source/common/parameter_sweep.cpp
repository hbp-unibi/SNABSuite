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

#include <algorithm>  // std::find
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>  // unlink file

#include <cypress/cypress.hpp>
#include "common/snab_base.hpp"
#include "common/snab_registry.hpp"
#include "parameter_sweep.hpp"
#include "util/read_json.hpp"
#include "util/utilities.hpp"

namespace SNAB {

std::vector<cypress::Json> ParameterSweep::generate_sweep_vector(
    const cypress::Json &source, const cypress::Json &target,
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
			if (i.key() == "repetitions" || i.key() == "snab_name") {
				continue;
			}
			auto splitted = Utilities::split(i.key(), '/');
			if (splitted.back() == "0") {
				std::string name;
				for (size_t i = 1; i < splitted.size() - 1; i++) {
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
			tar[i.key()] = val;
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

void ParameterSweep::shuffle_sweep_indices(size_t size)
{
	// Shuffle sweep indices for stochastic independence in simulations on
	// spikey
	std::default_random_engine generator(1010);
	m_indices = std::vector<size_t>(size);
	for (size_t j = 0; j < size; j++) {
		m_indices[j] = j;
	}
	std::shuffle(m_indices.begin(), m_indices.end(), generator);
}

using cypress::Json;
void ParameterSweep::recover_broken_simulation()
{
	// Open a the backup file, check if it exists
	std::fstream ss(m_backend + "_bak.json", std::fstream::in);
	if (!ss.good()) {
		return;
	}

	Json backup;
	try {
		backup = Json::parse(ss);
	}
	catch (...) {
		global_logger().info("SNABSuite",
		                     "Backup file exists, but is corrupt! "
		                     "Skipping recovery and overwriting old file");
		return;
	}
	// Check wether the backup is for the correct SNAB
	if (backup["snab"] != m_snab->snab_name()) {
		global_logger().info("SNABSuite",
		                     "Backup file exists, but not for this SNAB! "
		                     "Skipping recovery and overwriting old file");
		return;
	}

	// Check wether sweep indices have the same size
	if (m_indices.size() != backup["indices"].size()) {
		global_logger().info("SNABSuite",
		                     "Sweep size of backup is incorrect! Skipping "
		                     "recovery and overwriting old file");
		return;
	}

	// Check results dimension
	if (m_results.size() != backup["results"].size() ||
	    m_results[0].size() != backup["results"][0].size()) {
		global_logger().info("SNABSuite",
		                     "Results size of backup is incorrect! Skipping "
		                     "recovery and overwriting old file");
		return;
	}

	if (backup["jobs_done"].size() == 0) {
		global_logger().info("SNABSuite",
		                     "Empty backup file! Skipping "
		                     "recovery and overwriting old file");
		return;
	}

	// Recover data
	m_indices = json_array_to_vector<size_t>(backup["indices"]);
	m_results = json_2Darray_to_vector<cypress::Real>(backup["results"]);
	m_jobs_done = json_array_to_vector<size_t>(backup["jobs_done"]);

	// Check for invalid results (NaN) and repeat these experiments
	for (size_t i = 0; i < m_indices.size(); i++) {
		bool is_nan = false;
		for (auto j : m_results[i]) {
			if (std::isnan(j)) {
				is_nan = true;
				break;
			}
		}
		if (is_nan) {
			auto iter =
			    std::find(m_jobs_done.begin(), m_jobs_done.end(), m_indices[i]);
			if (iter != m_jobs_done.end()) {
				m_jobs_done.erase(iter);
			}
		}
	}
	
	std::cout << "Succesfully recovered old parameter sweep!" << std::endl;
}

void ParameterSweep::backup_simulation_results()
{
	if (m_jobs_done.size() == 0) {
		global_logger().info("SNABSuite",
		                     "No simulation finished! Skipping backup.");
		return;
	}

	// Put together the backup of all important data
	Json backup;
	backup["snab"] = m_snab->snab_name();
	backup["indices"] = m_indices;
	backup["results"] = m_results;
	backup["jobs_done"] = m_jobs_done;

	// Write to file
	std::fstream ss(m_backend + "_bak.json", std::fstream::out);
	ss << backup.dump(0) << std::endl;
	ss.close();
}

ParameterSweep::ParameterSweep(std::string backend, cypress::Json &config,
                               size_t bench_index)
    : m_backend(backend)
{
	std::string snab_name = config["snab_name"];
	m_sweep_config = extract_backend(config, m_backend);
	// Get the correct SNAB
	auto snab_vec = snab_registry(m_backend, bench_index);
	for (auto i : snab_vec) {
		if (i->snab_name() == snab_name) {
			m_snab = i;
		}
	};
	if (m_sweep_config.find("repetitions") != m_sweep_config.end()) {
		// TODO Feature still missing
		// m_repetitions = m_sweep_config["repetitions"];
	}

	m_sweep_vector = generate_sweep_vector(m_sweep_config, m_snab->get_config(),
	                                       m_sweep_names);
	shuffle_sweep_indices(m_sweep_vector.size());
	m_results = std::vector<std::vector<cypress::Real>>(
	    m_indices.size(),
	    std::vector<cypress::Real>(m_snab->indicator_names().size(), 0));
	recover_broken_simulation();
}

void ParameterSweep::execute()
{

	size_t backup_count = 0;

	for (size_t i = 0; i < m_indices.size(); i++) {
		// Report the percentage of jobs done
		Utilities::progress_callback(double(i) / double(m_indices.size()));

		// Get the new index
		size_t current_index = m_indices[i];
		// Check if simulation has been done in previous (broken) simulation
		if (std::find(m_jobs_done.begin(), m_jobs_done.end(), current_index) !=
		    m_jobs_done.end()) {
			continue;
		}
		// Resetting the cypress::network structure in the SNAB
		m_snab->reset_network();
		m_snab->set_config(m_sweep_vector[current_index]);
		m_snab->build();
		m_snab->run();
		m_results[i] = m_snab->evaluate();
		// Add the current job to the list of finished indices
		m_jobs_done.emplace_back(current_index);
		backup_count++;
		if (backup_count >= 50) {
			backup_simulation_results();
			backup_count = 0;
		}
	}
	// Finalize output in terminal
	Utilities::progress_callback(1.0);
	std::cerr << std::endl;
}

// Accessing a unflattend JSON with flattend key
cypress::Real get_value_with_flattened_key(const cypress::Json &json,
                                           std::string &key)
{
	std::vector<std::string> splitted = Utilities::split(key, '/');
	cypress::Json json_temp = json[splitted[1]];

	for (size_t i = 2; i < splitted.size(); i++) {
		json_temp = json_temp[splitted[i]];
	}
	return json_temp;
}

void ParameterSweep::write_csv()
{
	// Get the direct parameter names without json key
	std::vector<std::string> shortened_sweep_names;
	for (auto i : m_sweep_names) {
		shortened_sweep_names.push_back(Utilities::split(i, '/').back());
	}

	// Put the sweep parameters into the results structure
	for (size_t i = 0; i < m_results.size(); i++) {
		for (size_t j = 0; j < m_sweep_names.size(); j++) {
			m_results[i].emplace_back(get_value_with_flattened_key(
			    m_sweep_vector[m_indices[i]], m_sweep_names[j]));
		}
	}

	// Sort the structure for the last entries (sweep parameters)
	size_t sweep_size = m_sweep_names.size();
	std::sort(m_results.begin(), m_results.end(),
	          [&sweep_size](const std::vector<cypress::Real> a,
	                        const std::vector<cypress::Real> b) {
		          size_t size = a.size();
		          for (size_t i = 1; i <= sweep_size; i++) {
			          if (a[size - i] != b[size - i]) {
				          return a[size - i] < b[size - i];
			          }
		          }
		          return a.back() < b.back();
		      });

	std::string filename = m_snab->snab_name() + "/";

	int dir_err =
	    system((std::string("mkdir -p ") + m_snab->snab_name()).c_str());
	if (dir_err == -1) {
		std::cout << "Error creating directory!" << std::endl;
		filename = "";
	}
	for (auto i : shortened_sweep_names) {
		filename += i + "_";
	}
	filename += Utilities::split(m_backend, '=')[0] + ".csv";
	std::ofstream ofs(filename, std::ofstream::out);
	if (!ofs.good()) {
		std::cout << "Error creating CSV" << std::endl;
	}
	// first line of csv
	ofs << "#";
	for (size_t i = 1; i <= sweep_size; i++) {
		ofs << shortened_sweep_names[sweep_size - i] << ",";
	}
	auto indicator_names = m_snab->indicator_names();
	for (size_t i = 1; i <= indicator_names.size(); i++) {
		ofs << indicator_names[indicator_names.size() - i] << ",";
	}
	ofs << "\n";
	for (size_t i = 0; i < m_results.size(); i++) {
		for (int j = m_results[0].size() - 1; j >= 0; j--) {
			ofs << m_results[i][j] << ",";
		}
		ofs << "\n";
	}
	ofs.close();

	// Remove backup file
	unlink((m_backend + "_bak.json").c_str());
}

ParameterSweep::~ParameterSweep() = default;
}
