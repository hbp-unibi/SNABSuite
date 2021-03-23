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

#include "benchmark.hpp"

#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;

namespace {

void collapse_entry(const std::vector<Json> &results, Json &tar,
                    std::string key, size_t index)
{
	tar[key + "_vec"] = std::vector<double>();
	size_t counter = 0;
	for (const Json &entry : results) {
		if (entry[index].count(key) > 0) {
			tar[key + "_vec"].emplace_back(entry[index][key]);
			counter++;
		}
	}
	if (counter > 0) {
		double min, max, avg, std_dev;
		Utilities::calculate_statistics(
		    tar[key + "_vec"].get<std::vector<double>>(), min, max, avg,
		    std_dev);
		tar[key] = avg;
		tar[key + "_min"] = min;
		tar[key + "_max"] = max;
		tar[key + "_std_dev"] = std_dev;
	}
	else {
		tar.erase(key + "_vec");
		tar.erase(key);
	}
}
}  // namespace

Json BenchmarkExec::merge_repeat_results(const std::vector<Json> &results)
{
	Json final_res;
	// results[i] is array!
	size_t n_entries = results[0].size();
	for (size_t i = 0; i < n_entries; i++) {
		Json res;
		res["name"] = results[0][i]["name"];
		res["type"] = results[0][i]["type"];
		res["measure"] = results[0][i]["measure"];
		if (results[0][i].count("units")) {
			res["units"] = results[0][i]["units"];
		}
		collapse_entry(results, res, "value", i);
		collapse_entry(results, res, "std_dev", i);
		collapse_entry(results, res, "min", i);
		collapse_entry(results, res, "max", i);
		final_res.push_back(res);
	}
	return final_res;
}
}  // namespace SNAB
