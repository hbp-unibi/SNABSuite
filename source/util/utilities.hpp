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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace SNAB {
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
};
}

#endif /* SNABSUITE_UTIL_UTILITIES_HPP */
