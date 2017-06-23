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

#pragma once

#ifndef SNABSUITE_UTIL_UTILITIES_HPP
#define SNABSUITE_UTIL_UTILITIES_HPP

#include <algorithm>  // Minimal and Maximal element
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>  // std::accumulate
#include <sstream>
#include <string>
#include <vector>

#include <cypress/cypress.hpp>

namespace SNAB {

using cypress::Json;

class Utilities {
public:
	/**
	 * Splits a string @param s into parts devided by @param delim and stores
	 * the
	 * result in @param elems and returns it
	 */
	static std::vector<std::string> &split(const std::string &s, char delim,
	                                       std::vector<std::string> &elems)
	{
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
		return elems;
	}

	/**
	 * The same as above, but only returning the vector.
	 */
	static std::vector<std::string> split(const std::string &s, char delim)
	{
		std::vector<std::string> elems;
		split(s, delim, elems);
		return elems;
	}

	/**
	 * Funtion for generating a progress bar on terminal
	 */
	static void progress_callback(double p)
	{
		const int w = 50;
		std::cerr << std::fixed << std::setprecision(2) << std::setw(6)
		          << p * 100.0 << "% [";
		const int j = p * float(w);
		for (int i = 0; i < w; i++) {
			std::cerr << (i > j ? ' ' : (i == j ? '>' : '='));
		}
		std::cerr << "]\r";
	}

	/**
	 * Calculating statistics of a vector, using an estimator for std_dev
	 * (sample covariance)
	 * @param data contains sample data
	 * @param min, @param max provide minimal, maximal values
	 * @param avg sample average
	 * @param std_dev smaple covariance
	 */
	template <typename T>
	static void calculate_statistics(const std::vector<T> &data, T &min, T &max,
	                                 double &avg, double &std_dev)
	{
		if (data.size() > 1) {
			// Calculate statistics
			max = *std::max_element(data.begin(), data.end());
			min = *std::min_element(data.begin(), data.end());
			avg = double(std::accumulate(data.begin(), data.end(), 0.0)) /
			      double(data.size());
			std_dev = 0.0;
			std::for_each(data.begin(), data.end(), [&](const T val) {
				std_dev += (val - avg) * (val - avg);
			});
			std_dev = std::sqrt(double(std_dev) / double(data.size() - 1));
		}
		else if (data.size() == 1) {
			min = data[0];
			max = data[0];
			avg = data[0];
			std_dev = 0;
		}
		else {
			min = 0;
			max = 0;
			avg = 0;
			std_dev = 0;
		}
	}

	template <typename T>
	static void write_vector_to_csv(T &data, std::string file_name)
	{
		auto file = std::ofstream(file_name, std::ofstream::out);
		if (file.good()) {
			for (auto i : data) {
				file << i << std::endl;
			}
			// file << std::endl;
		}
		else {
			std::cerr << "Could not open file " << file_name
			          << "for writing csv!" << std::endl;
		}
	}

	template <typename T>
	static void write_vector2_to_csv(T &data, std::string file_name)
	{
		auto file = std::ofstream(file_name, std::ofstream::out);
		if (file.good()) {
			for (auto i : data) {
				for (auto j : i) {
					file << j << ", ";
				}
				file << std::endl;
			}
		}
		else {
			std::cerr << "Could not open file " << file_name
			          << "for writing csv!" << std::endl;
		}
	}

	/**
	 * Merge two json objects. Note: Values already included in @param a will be
	 * overwritten!
	 * See https://github.com/nlohmann/json/issues/252 for source.
	 * @return combined json object
	 */
	static Json merge_json(const Json &a, const Json &b)
	{
		Json result = a.flatten();
		Json tmp = b.flatten();

		for (Json::iterator it = tmp.begin(); it != tmp.end(); ++it) {
			result[it.key()] = it.value();
		}

		return result.unflatten();
	}

	/**
	 * Merge the backend strings with a provided json object.
	 * Note: options already included in backend will not be overwritten!
	 * @param backend: string containig "[backend]" (+ "{setup options}"), will
	 * contain combined setup after execution
	 * @param json: Object containig additional options for backend
	 * @return: Combined json object
	 */
	static Json manipulate_backend_string(std::string &backend, Json &json)
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
};
}

#endif /* SNABSUITE_UTIL_UTILITIES_HPP */
