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

#include <cypress/cypress.hpp>

#include <string>

#include "gtest/gtest.h"

#include "common/neuron_parameters.hpp"
#include "util/spiking_utils.hpp"

namespace SNAB {
TEST(SpikingUtils, detect_type)
{
	EXPECT_EQ(&SpikingUtils::detect_type("IF_cond_exp"), &IfCondExp::inst());
	EXPECT_EQ(&SpikingUtils::detect_type("IfFacetsHardware1"),
	          &IfFacetsHardware1::inst());
	EXPECT_EQ(&SpikingUtils::detect_type("AdExp"), &EifCondExpIsfaIsta::inst());
	EXPECT_ANY_THROW(SpikingUtils::detect_type("Nothing"));
}

TEST(SpikingUtils, add_population)
{
	cypress::Network netw;
	cypress::Json json;
	NeuronParameters params(IfCondExp::inst(), json);
	std::string type = "IF_cond_exp";
	auto pop1 = SpikingUtils::add_population(type, netw, params, 1);
	EXPECT_EQ(netw.population_count(), size_t(1)

	);
	EXPECT_EQ(netw.neuron_count(), size_t(1));
	EXPECT_EQ(netw.neuron_count(IfCondExp::inst()), size_t(1)

	);
	EXPECT_EQ(netw.neuron_count(IfFacetsHardware1::inst()), size_t(0)

	);
	EXPECT_FALSE(pop1.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop1.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));

	EXPECT_EQ(pop1.size(), size_t(1));
	EXPECT_EQ(&pop1.type(), &IfCondExp::inst());

	auto pop2 = SpikingUtils::add_population(type, netw, params, 1, "v");
	EXPECT_TRUE(pop2.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop2.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));

	auto pop3 = SpikingUtils::add_population(type, netw, params, 1, "gsyn_exc");
	EXPECT_FALSE(pop3.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop3.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));
}

TEST(SpikingUtils, calc_num_spikes)
{
	std::vector<cypress::Real> spiketrain;
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 0.0), 0);
	for (size_t i = 0; i < 50; i++) {
		spiketrain.push_back(cypress::Real(i) * 0.3);
	}
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 20.0), 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 15.0), 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.7), 1);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.4), 2);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.0), 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 13.6), 4);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 0.0), 50);

	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 11.4), 05);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 11.7), 06);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 12.0), 07);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 13.0), 10);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 14.0), 13);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 15.0), 16);
}

TEST(SpikingUtils, calc_num_spikes_vec)
{
	std::array<cypress::Real, 50> spiketrain, spiketrain2;
	for (size_t i = 0; i < 50; i++) {
		spiketrain[i] = cypress::Real(i) * 0.3;
		spiketrain2[i] = cypress::Real(i) * 0.4;
	}
	cypress::Matrix<cypress::Real> spiketrains(
	    std::array<std::array<cypress::Real, 50>, 2>{
	        {spiketrain, spiketrain2}});

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 20.0)[0], 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 15.0)[0], 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.7)[0], 1);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.4)[0], 2);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.0)[0], 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 13.6)[0], 4);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains)[0], 50);

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 11.4)[0], 5);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 11.7)[0], 6);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 12.0)[0], 7);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 13.0)[0],
	          10);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 14.0)[0],
	          13);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 15.0)[0],
	          16);

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 0.0)[1], 50);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 8.0, 9.0)[1], 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 8.0, 10.0)[1], 6);
}

TEST(SpikingUtils, spike_time_binning)
{
	std::vector<Real> spikes = {};
	auto bins = SpikingUtils::spike_time_binning(0, 1, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(0), i);
	}

	for (size_t i = 0; i < 100; i++) {
		spikes.push_back(i * 0.1);
	}
	bins = SpikingUtils::spike_time_binning(0, 10, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}

	bins = SpikingUtils::spike_time_binning(1, 10, 9, spikes);
	EXPECT_EQ(size_t(9), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}
	bins = SpikingUtils::spike_time_binning(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}

	bins = SpikingUtils::spike_time_binning(1, 9, 16, spikes);
	EXPECT_EQ(size_t(16), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(5), i);
	}
	spikes.push_back(3.421);
	spikes.push_back(8.95);
	std::sort(spikes.begin(), spikes.end());
	bins = SpikingUtils::spike_time_binning(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());

	EXPECT_EQ(size_t(11), bins[2]);
	EXPECT_EQ(size_t(11), bins[7]);
}
}// namespace SNAB
