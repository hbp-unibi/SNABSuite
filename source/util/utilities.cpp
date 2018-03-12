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
#include "utilities.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cypress/cypress.hpp>

namespace SNAB {
using cypress::Json;
std::vector<std::string> &Utilities::split(const std::string &s, char delim,
                                           std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> Utilities::split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

void Utilities::progress_callback(double p)
{
	const int w = 50;
	std::cerr << std::fixed << std::setprecision(2) << std::setw(6) << p * 100.0
	          << "% [";
	const int j = p * float(w);
	for (int i = 0; i < w; i++) {
		std::cerr << (i > j ? ' ' : (i == j ? '>' : '='));
	}
	std::cerr << "]\r";
}
namespace {
inline bool ends_with(std::string const &value, std::string const &ending)
{
	if (ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
}  // namespace

Json Utilities::merge_json(const Json &a, const Json &b)
{
	Json result = a.flatten();
	Json tmp = b.flatten();

	for (Json::iterator it = tmp.begin(); it != tmp.end(); ++it) {
		if (ends_with(it.key(), "/0")) {
			std::string tmp = it.key();
			tmp.pop_back();
			tmp.pop_back();
			if (result.find(tmp) != result.end()) {
				result.erase(result.find(tmp));
			}
		}
		result[it.key()] = it.value();
	}

	return result.unflatten();
}

Json Utilities::manipulate_backend_string(std::string &backend, Json &json)
{
	Json res;

	// Check wether there are options given in the backend string
	if (split(backend, '=').size() > 1) {
		Json old = Json::parse(split(backend, '=')[1]);
		res = merge_json(json, old);
	}
	else {
		res = json;
	}
	// Construct the new backend
	backend = split(backend, '=')[0] + "=" + res.dump(-1);

	// Return the merged json
	return res;
}

void Utilities::plot_spikes(std::string filename, std::string simulator)
{
	std::string short_sim = split(split(simulator, '=')[0], '.').back();
	try {
		system(("../plot/spike_plot.py " + filename + " -s " + short_sim + "&")
		           .c_str());
	}
	catch (...) {
		std::cerr << "Calling spike_plot.py caused an error!" << std::endl;
	}
}

void Utilities::plot_histogram(std::string filename, std::string simulator,
                               bool normalized, int n_bins, std::string title)
{
	std::string short_sim = split(split(simulator, '=')[0], '.').back();
	try {
		if (n_bins < 0 && normalized) {
			system(("../plot/histogram.py " + filename + " -s " + short_sim +
			        " -t " + title + " -n " + "&")
			           .c_str());
		}
		else if (n_bins < 0 && !normalized) {
			system(("../plot/histogram.py " + filename + " -s " + short_sim +
			        " -t " + title + "&")
			           .c_str());
		}
		else if (n_bins > 0 && normalized) {
			system(("../plot/histogram.py " + filename + " -s " + short_sim +
			        " -t " + title + " -b " + std::to_string(n_bins) + " -n " +
			        "&")
			           .c_str());
		}
		else if (n_bins > 0 && !normalized) {
			system(("../plot/histogram.py " + filename + " -s " + short_sim +
			        " -t " + title + " -b " + std::to_string(n_bins) + "&")
			           .c_str());
		}
	}
	catch (...) {
		std::cerr << "Calling spike_plot.py caused an error!" << std::endl;
	}
}

void Utilities::plot_voltages_spikes(std::string filename,
                                     std::string simulator, size_t mem_col,
                                     size_t t_col, std::string spikes_file,
                                     size_t spikes_col)
{
	std::string short_sim = split(split(simulator, '=')[0], '.').back();
	std::string exec = "../plot/plot_membrane_pot.py " + filename + " -s " +
	                   short_sim + " -tc " + std::to_string(t_col) + " -y" +
	                   std::to_string(mem_col);
	if (spikes_file != "") {
		exec = exec + " -sp " + spikes_file + " -spc " +
		       std::to_string(spikes_col);
	}
	try {
		system((exec + " &").c_str());
	}
	catch (...) {
		std::cerr << "Calling spike_plot.py caused an error!" << std::endl;
	}
}
}  // namespace SNAB
