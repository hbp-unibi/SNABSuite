#pragma once
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

#include "energy/um25c.hpp"
#include "util/utilities.hpp"

namespace Energy {

/**
 * @brief Measure Power usage via nvidia-smi tool. Must be installed on the
 * target system!
 *
 */
class nvidiasmi : public MeasureDevice {
private:
	const std::string m_call = "nvidia-smi --query-gpu=power.draw --format=csv";

public:
	nvidiasmi() = default;
	double read()
	{
		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&pclose)> pipe(
		    popen(m_call.c_str(), "r"), pclose);
		if (!pipe) {
			throw std::runtime_error("popen() failed!");
		}
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}
		result = SNAB::Utilities::split(result, '\n')[1];
		return std::stod(result);
	}

	virtual data get_data_sample_timed() override
	{
		double val = read() * 1.e3;  // milliwatt
		// Assume general 12V supply lane
		return std::make_tuple<
		    std::chrono::time_point<std::chrono::steady_clock>, double, double,
		    double>(std::chrono::steady_clock::now(), 12.0 * 1.e3, val / 12.0,
		            val*1.);
	}
};
}  // namespace Energy
