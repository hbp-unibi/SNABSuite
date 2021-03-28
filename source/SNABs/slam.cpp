/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2020  Christoph Ostrau
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
#include <cypress/backend/power/power.hpp>  // Control of power via netw
#include <cypress/cypress.hpp>              // Neural network frontend
#include <vector>

#include "simulation.h"
#include "slam.hpp"
#include "spikingnetwork.h"
#include "util/utilities.hpp"

namespace SNAB {
using namespace cypress;

SpikingSlam::SpikingSlam(const std::string backend, size_t bench_index)
    : SNABBase(
          __func__, backend, {"false positives", "false negatives"},
          {"quality", "quality"}, {"FP", "FN"}, {"", ""},
          {"neuron_params", "neuron_params_2", "neuron_params_3", "wtaParams",
           "stParams", "common", "psParams", "others", "sim_path", "scale_th"},
          bench_index)
{
}

namespace {
/**
 * @brief return a map of the given simulation
 * True: Obstacle or wall
 * False: Free point
 *
 * @param path Path to the simulation. Can be produced by using the SLAM GUI
 * @return std::vector< std::std::vector< bool > > Map
 */
std::vector<std::vector<bool>> get_map(const std::string &path)
{

	std::ifstream ifs(path);
	if (!ifs.good()) {
		throw;
	}
	Json config;
	ifs >> config;
	return config["map"].get<std::vector<std::vector<bool>>>();
}

/**
 * @brief Check if any of the surrounding points contains an obstacle -->
 * activating the bumper sensor
 *
 * @param map the original simulation map
 * @param current_x Current x position
 * @param current_y Current y position
 * @return bool true if any surrounding point is an obstacle
 */
inline bool check_surrounding(const std::vector<std::vector<bool>> &map,
                              const size_t &current_x, const size_t &current_y)
{
	if (map[current_x + 1][current_y]) {
		return true;
	}
	if (map[current_x - 1][current_y]) {
		return true;
	}
	if (map[current_x][current_y + 1]) {
		return true;
	}
	if (map[current_x][current_y - 1]) {
		return true;
	}
	if (map[current_x + 1][current_y + 1]) {
		return true;
	}
	if (map[current_x - 1][current_y - 1]) {
		return true;
	}
	if (map[current_x - 1][current_y + 1]) {
		return true;
	}
	if (map[current_x + 1][current_y - 1]) {
		return true;
	}
	if (map[current_x][current_y]) {
		return false;
	}
	return false;
}

/**
 * @brief Converts the original map to the map that is actually learned -->
 * Points where the bumper sensor is activated are true, walls are not included
 * anymore as the robot cannot drive into the wall. Thus dimensions are reduced
 * by 2.
 *
 * @param map Original simulation map
 * @return std::vector< std::std::vector< bool > > converted Map
 */
std::vector<std::vector<bool>> convert_map(
    const std::vector<std::vector<bool>> &map)
{
	std::vector<std::vector<bool>> res(map.size() - 2,
	                                   std::vector<bool>(map[0].size() - 2));
	for (size_t i = 0; i < res.size(); i++) {
		for (size_t j = 0; j < res[0].size(); j++) {
			size_t j_inv = res[0].size() - j - 1;
			res[i][j] = check_surrounding(map, i + 1, j_inv + 1);
		}
	}
	return res;
}
}  // namespace

Network &SpikingSlam::build_netw(cypress::Network &netw)
{

	SpikingNetwork slam_netw("", m_config_file, 8, 15, 15, 1, 1);
	slam_netw.createNetwork(netw, m_config_file["sim_path"].get<std::string>());
	m_map = get_map(m_config_file["sim_path"].get<std::string>());
	m_conn = netw.connection("stdp");
	xsize = netw.populations("X")[0].size();
	ysize = netw.populations("Y")[0].size();
	m_scale_th = m_config_file["scale_th"];
	return netw;
}

void SpikingSlam::run_netw(cypress::Network &netw)
{
	cypress::PowerManagementBackend pwbackend(
	    cypress::Network::make_backend(m_backend));
	netw.run(pwbackend);
}

#if SNAB_DEBUG
namespace {
void plotPopulation(Network &netw, std::string popname, std::string filename,
                    std::string backend)
{
	std::vector<std::vector<Real>> spikes = {};
	for (size_t j = 0; j < netw.populations(popname)[0].size(); j++) {
		spikes.push_back(netw.populations(popname)[0][j].signals().data(0));
	}

	Utilities::write_vector2_to_csv(spikes, filename + "_" + popname + ".csv");
	Utilities::plot_spikes(filename + "_" + popname + ".csv", backend);
}
}  // namespace
#endif

std::vector<std::array<cypress::Real, 4>> SpikingSlam::evaluate()
{
	const auto &weights_ = m_conn.connector().learned_weights();

	// Read in weights
	std::vector<Real> weight(weights_.size());
	if (xsize * ysize == weights_.size()) {
		for (size_t i = 0; i < xsize; i++) {
			for (size_t j = 0; j < ysize; j++) {
				size_t y_inv = ysize - j - 1;
				weight[y_inv * xsize + i] =
				    weights_[j * xsize + i].SynapseParameters[0];
			}
		}
	}
	else {
		throw std::runtime_error(
		    "Somehow the size the learned map is different from the original "
		    "map!");
	}
	Real max = *std::max_element(weight.begin(), weight.end());
	Real min = *std::min_element(weight.begin(), weight.end());
	Real thresh = min + (m_scale_th * (max - min));  // Threshhold for map
	auto map2 = convert_map(m_map);
	size_t fp = 0, fn = 0;  // Convert the false set entries in learnt map

	// m_map is shifted: it has dimension xsize + 2, ysize + 2 to include
	// boarders!
	for (size_t j = 0; j < ysize; j++) {
		for (size_t i = 0; i < xsize; i++) {

			if (weight[j * xsize + i] > thresh) {
				if (!map2[i][j]) {
					fp++;
#if SNAB_DEBUG
					std::cout << "O";
				}
				else {
					std::cout << "X";
#endif
				}
			}
			else {
				if (map2[i][j]) {
					fn++;
#if SNAB_DEBUG
					std::cout << "B";
				}
				else {
					std::cout << " ";
#endif
				}
			}
		}
#if SNAB_DEBUG
		std::cout << std::endl;
#endif
	}
#if SNAB_DEBUG
	std::vector<float> data(weight.size());
	for (size_t i = 0; i < data.size(); i++) {
		data[i] = weight[i];
	}
	std::vector<float> data2(xsize * ysize);
	for (size_t i = 0; i < data.size(); i++) {
		data2[i] = weight[i] > thresh ? max : min;
	}
	std::vector<float> data3(xsize * ysize);
	for (size_t i = 0; i < xsize; i++) {
		for (size_t j = 0; j < ysize; j++) {
			data3[j * xsize + i] = map2[i][j] ? max : min;
		}
	}
	Utilities::write_vector2_to_csv(map2, _debug_filename("TargetMap.csv"));
	Utilities::write_vector_to_csv(weight, _debug_filename("LearntMap.csv"));
	Utilities::write_vector_to_csv(data2, _debug_filename("LearntMapTh.csv"));
	system(("../plot/plot_map.py " + _debug_filename("") + " -o " +
	        _debug_filename("") + "map.pdf &")
	           .c_str());

	plotPopulation(m_netw, "HD", _debug_filename("SLAM"), m_backend);
	plotPopulation(m_netw, "X", _debug_filename("SLAM"), m_backend);
	plotPopulation(m_netw, "Y", _debug_filename("SLAM"), m_backend);
	plotPopulation(m_netw, "POS", _debug_filename("SLAM"), m_backend);
	plotPopulation(m_netw, "CON", _debug_filename("SLAM"), m_backend);
#endif
	return {{double(fp), NaN(), NaN(), NaN()},
	        {double(fn), NaN(), NaN(), NaN()}};
}
}  // namespace SNAB
