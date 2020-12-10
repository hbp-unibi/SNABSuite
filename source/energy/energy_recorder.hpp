
#pragma once

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "um25c.hpp"
namespace Energy {
std::atomic<bool> m_record(false);

/**
 * @brief This class is not thread safe! Would not make any sense since there is
 * only one recording device in one instance!
 *
 */
class Multimeter {
private:
	um25c m_device;
	static inline void check_size(size_t size)
	{
		if (!size) {
			throw std::runtime_error(
			    "Require energy record with more than one entry!");
		}
	}

	using timed_record = um25c::timed_record;

	std::atomic<bool> m_record{false};
	std::thread m_thread;
	std::vector<timed_record> m_data;

	void priv_continuos_record()
	{
		while (m_record) {
			m_data.emplace_back(m_device.get_data_sample_timed());
		}
	}

public:
	Multimeter(std::string port = "/dev/rfcomm0") : m_device(port) {}

	std::vector<um25c::timed_record> continuos_record(std::atomic<bool> &record)
	{
		std::vector<timed_record> res;
		while (record) {
			res.emplace_back(m_device.get_data_sample_timed());
		}
		return res;
	}

	std::vector<timed_record> record(size_t n_samples)
	{
		std::vector<timed_record> res(n_samples);
		for (size_t i = 0; i < n_samples; i++) {
			res[i] = m_device.get_data_sample_timed();
		}
		return res;
	}

	void start_recording()
	{
		if (m_record.load()) {
			throw std::runtime_error(
			    "Call to start_recording while already recording");
		}
		m_record.store(true);  // Not threadsafe, could be true in meantime
		m_data.clear();
		m_thread = std::thread(&Multimeter::priv_continuos_record, this);
	}

	const std::vector<timed_record> &stop_recording()
	{
		if (!m_record.load()) {
			throw std::runtime_error(
			    "Call to stop_recording without recording");
		}
		m_record.store(false);
		m_thread.join();
		return m_data;
	}

	typedef std::tuple<std::chrono::duration<double, std::micro>, uint16_t,
	                   uint16_t, uint32_t>
	    data;

	static std::vector<data> convert_record(
	    const std::vector<timed_record> &rec)
	{
		check_size(rec.size());
		std::vector<data> res(rec.size());
		for (size_t i = 1; i < rec.size(); i++) {
			res[i] = {std::chrono::duration<double, std::micro>(
			              rec[i].second - rec[i - 1].second),
			          rec[i].first.millivolts, rec[i].first.tenths_milliamps,
			          rec[i].first.milliwatts};
		}
		return res;
	}

	static std::vector<data> convert_record_thresh(
	    const std::vector<timed_record> &rec, uint32_t milliamps_thresh)
	{
		check_size(rec.size());
		milliamps_thresh = milliamps_thresh * 10;
		std::vector<data> res;
		for (size_t i = 1; i < rec.size(); i++) {
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res.emplace_back(std::make_tuple(
				    std::chrono::duration<double, std::micro>(
				        rec[i].second - rec[i - 1].second),
				    rec[i].first.millivolts, rec[i].first.tenths_milliamps,
				    rec[i].first.milliwatts));
			}
		}
		return res;
	}

	/**
	 * @brief Calculates the energy expenditure of a measurement
	 *
	 * @param rec vector of samples
	 * @param milliamps_thresh (optional) threshold in milli amp
	 * @return Energy expenditure in mJoule
	 */
	static double calculate_energy(const std::vector<timed_record> &rec,
	                               uint32_t milliamps_thresh = 0.0)
	{
		check_size(rec.size());
		double res = 0.0;
		milliamps_thresh *= 10;
		for (size_t i = 1; i < rec.size(); i++) {
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res += double(rec[i].first.milliwatts) *
				       std::chrono::duration<double>(rec[i].second -
				                                     rec[i - 1].second)
				           .count();
				// mWatt * second = mJoule
			}
		}
		return res;
	}

	double calculate_energy(uint32_t milliamps_thresh = 0.0) const
	{
		return calculate_energy(m_data, milliamps_thresh);
	}

	static double calculate_energy_last(const std::vector<timed_record> &rec,
	                                    uint32_t milliamps_thresh)
	{
		check_size(rec.size());
		std::vector<double> res({0.0});
		milliamps_thresh *= 10;
		for (size_t i = 1; i < rec.size(); i++) {
			// 			std::cout << rec[i].first.tenths_milliamps << ", "
			// 			          << milliamps_thresh << ", "
			// 			          << std::chrono::duration<double>(rec[i].second
			// - 			                                           rec[i -
			// 1].second) 			                 .count()
			// 			          << ", " << double(rec[i].first.milliwatts) <<
			// std::endl;
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res.back() += double(rec[i].first.milliwatts) *
				              std::chrono::duration<double>(rec[i].second -
				                                            rec[i - 1].second)
				                  .count();
				// mWatt * second = mJoule
			}
			else if (res.back() != 0) {
				res.emplace_back(0.0);
			}
		}

		if (res.back()) {
			return res.back();
		}
		else {
			if (res.size() == 1) {
				return 0.0;
			}
			auto index = res.size() - 2;
			return res[index];
		}
	}

	double calculate_energy_last(uint32_t milliamps_thresh) const
	{
		return calculate_energy_last(m_data, milliamps_thresh);
	}

	/**
	 * @brief Calculates the average power draw.
	 *
	 * @param rec vector of samples
	 * @param milliamps_thresh (optional) threshold in milli amp
	 * @return average power draw in milli watt
	 */
	static double average_power_draw(const std::vector<timed_record> &rec,
	                                 uint32_t milliamps_thresh = 0.0)
	{
		uint32_t res = 0.0;
		size_t counter = 0;
		milliamps_thresh = milliamps_thresh * 10.0;
		for (size_t i = 0; i < rec.size(); i++) {
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res += rec[i].first.milliwatts;
				counter++;
			}
		}
		return double(res) / double(counter);
	}

	double average_power_draw(uint32_t milliamps_thresh = 0.0) const
	{
		return average_power_draw(m_data, milliamps_thresh);
	}
	static double average_power_draw_last(const std::vector<timed_record> &rec,
	                                      uint32_t milliamps_thresh)
	{
		std::vector<uint32_t> res({0});
		std::vector<size_t> counter({0});
		milliamps_thresh = milliamps_thresh * 10.0;
		for (size_t i = 0; i < rec.size(); i++) {
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res.back() += rec[i].first.milliwatts;
				counter.back()++;
			}
			else if (res.back()) {
				res.emplace_back(0);
				counter.emplace_back(0);
			}
		}
		if (res.back()) {
			return double(res.back()) / double(counter.back());
		}
		else {
			if (res.size() == 1) {
				return 0.0;
			}
			auto index = res.size() - 2;
			return double(res[index]) / double(counter[index]);
		}
	}

	double average_power_draw_last(uint32_t milliamps_thresh) const
	{
		return average_power_draw_last(m_data, milliamps_thresh);
	}

	/**
	 * @brief Calculates the average recorded current
	 *
	 * @param rec vector of samples
	 * @param milliamps_thresh (optional) threshold in milli amp
	 * @return average current in milli ampere
	 */
	static double average_current(const std::vector<timed_record> &rec,
	                              uint32_t milliamps_thresh = 0.0)
	{
		uint32_t res = 0.0;
		size_t counter = 0;
		milliamps_thresh = milliamps_thresh * 10;
		for (size_t i = 0; i < rec.size(); i++) {
			if (rec[i].first.tenths_milliamps > milliamps_thresh) {
				res += rec[i].first.tenths_milliamps;
				counter++;
			}
		}
		return double(res) / double(counter * 10);
	}

	static double max_current(const std::vector<timed_record> &rec)
	{
		return (*std::max_element(rec.begin(), rec.end(),
		                          [](timed_record a, timed_record b) {
			                          return a.first.tenths_milliamps <
			                                 b.first.tenths_milliamps;
		                          }))
		           .first.tenths_milliamps /
		       10.0;
	}
	double max_current() const { return max_current(m_data); }

	static double max_voltage(const std::vector<timed_record> &rec)
	{
		return (*std::max_element(rec.begin(), rec.end(),
		                          [](timed_record a, timed_record b) {
			                          return a.first.millivolts <
			                                 b.first.millivolts;
		                          }))
		    .first.millivolts;
	}
	double max_voltage() const { return max_voltage(m_data); }

	static double min_current(const std::vector<timed_record> &rec)
	{
		return (*std::min_element(rec.begin(), rec.end(),
		                          [](timed_record a, timed_record b) {
			                          return a.first.tenths_milliamps <
			                                 b.first.tenths_milliamps;
		                          }))
		           .first.tenths_milliamps /
		       10.0;
	}
	double min_current() const { return min_current(m_data); }

	static double min_voltage(const std::vector<timed_record> &rec)
	{
		return (*std::min_element(rec.begin(), rec.end(),
		                          [](timed_record a, timed_record b) {
			                          return a.first.millivolts <
			                                 b.first.millivolts;
		                          }))
		    .first.millivolts;
	}
	double min_voltage() const { return min_voltage(m_data); }
};
}  // namespace Energy
