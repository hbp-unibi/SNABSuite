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

#include <cypress/cypress.hpp>

#include <algorithm>  // Minimal and Maximal element
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>  // std::accumulate
#include <string>
#include <vector>

namespace SNAB {

using cypress::Json;

/**
 * @brief Collection of usefull Utilities not directly related to spiking
 * networks
 *
 */
class Utilities {
public:
	/**
	 * @brief Splits a string s into parts devided by delim and stores
	 * the result in elems and returns it
	 *
	 * @param s string to be splitted
	 * @param delim char which seperated the strings
	 * @param elems container in which result is appended at the end
	 * @return the new elems
	 */
	static std::vector<std::string> &split(const std::string &s, char delim,
	                                       std::vector<std::string> &elems);
	/**
	 * @brief The same as Utilities::split, but only returning the vector.
	 *
	 * @param s string to be splitted
	 * @param delim char which seperated the strings
	 */
	static std::vector<std::string> split(const std::string &s, char delim);

	/**
	 * @brief Funtion for generating a progress bar on terminal
	 *
	 * @param p value between 0 and 1 representing current progress
	 */
	static void progress_callback(double p);

	/**
	 * @brief Calculating statistics of a vector, using an estimator for std_dev
	 * (sample covariance)
	 *
	 * @param data contains sample data
	 * @param min object for minimal value
	 * @param max object for maximal value
	 * @param avg object for sample average
	 * @param std_dev object for smaple covariance
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
	/**
	 * @brief Writes a vector to a csv
	 *
	 * @param T Type of data e.g. vector of ints, iterator musst support '<<'
	 * for output
	 * @param data Reference to the data
	 * @param file_name string containing filename, make sure folders already
	 * exists
	 */
	static void write_vector_to_csv(T &data, std::string file_name)
	{
		std::ofstream file;
		file.open(file_name, std::ofstream::out);
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
	/**
	 * @brief Writes a 2D vector to a csv
	 *
	 * @param T Type of data e.g. vector of a vector of ints, iterator musst
	 * support '<<' for output
	 * @param data Reference to the data
	 * @param file_name string containing filename, make sure folders already
	 * exists
	 */
	static void write_vector2_to_csv(T &data, std::string file_name,
	                                 std::string first_line = "")
	{
		std::ofstream file;
		file.open(file_name, std::ofstream::out);
		if (file.good()) {
			if (first_line != "") {
				file << first_line << std::endl;
			}
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
	 * @brief Merge two json objects. Note: Values already included in a will be
	 * overwritten!
	 * See [github](https://github.com/nlohmann/json/issues/252) for source.
	 *
	 * @param a target json, keys will b overwritten
	 * @param b source json, will be copied into a
	 * @return combined json object
	 */
	static Json merge_json(const Json &a, const Json &b);

	/**
	 * @brief Merge the backend strings with a provided json object.
	 * Note: options already included in backend will not be overwritten!
	 *
	 * @param backend string containig "[backend]" (+ "={setup options}"), will
	 * contain combined setup after execution
	 * @param json Object containig additional options for backend
	 * @return: Combined json object
	 */
	static Json manipulate_backend_string(std::string &backend, Json &json);

	/**
	 * @brief Given a filename of a csv with list of spikes this will produce a
	 * raster plot
	 *
	 * @param filename name and path to a csv
	 * @param simulator simulator string, simulator options will be stripped
	 * inside
	 */
	static void plot_spikes(std::string filename, std::string simulator);

	/**
	 * @brief Plotting a histogram a one dimensional data in a csv
	 *
	 * @param filename File and path to the csv
	 * @param simulator simulator string, options will be stripped insider
	 * @param normalized flag whether histogram should be normalized
	 * @param n_bins  number of bins for the histogram, negative values use
	 * 'auto' in pyplot
	 * @param title title to be plotted on top of histogram
	 */
	static void plot_histogram(std::string filename, std::string simulator,
	                           bool normalized, int n_bins, std::string title);

	/**
	 * @brief Plotting membrane voltage and (optional) plotting vertical lines
	 * for spikes on top
	 *
	 * @param filename name of the file containing membrane voltage, csv style
	 * @param simulator the (unshortened) backend/simulator string
	 * @param mem_col column containing the membrane potential (default = 1)
	 * @param t_col column containing the time values (default = 0)
	 * @param spikes_file (optional) file containing spikes of the same neuron
	 * @param spikes_col column of spike data in spikes_file
	 */
	static void plot_voltages_spikes(std::string filename,
	                                 std::string simulator, size_t mem_col = 1,
	                                 size_t t_col = 0,
	                                 std::string spikes_file = "",
	                                 size_t spikes_col = 0);

	/**
	 * @brief Plotting a curve with optional standard deviation
	 *
	 * @param filename file containing data
	 * @param simulator the (unshortened) backend/simulator string
	 * @param x_col column of x values
	 * @param y_col column of y values
	 * @param std_dev_vol column of standard deviation values
	 */
	static void plot_1d_curve(std::string filename, std::string simulator,
	                          size_t x_col, size_t y_col, int std_dev_vol = -1);
};
}  // namespace SNAB

#endif /* SNABSUITE_UTIL_UTILITIES_HPP */
