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
#include "common/parameter_sweep.hpp"

#include <sstream>
#include <vector>

#include "gtest/gtest.h"

namespace SNAB {
using cypress::Real;

static const std::string test_json =
    "{\n"
    "\t\"neuron_params\": {\n"
    "\t\t\"e_rev_E\": 0.0,\n"
    "\t\t\"v_rest\": -70.0,\n"
    "\t\t\"v_reset\": -80.0,\n"
    "\t\t\"v_thresh\": -57.0,\n"
    "\t\t\"tau_syn_E\": 2.0,\n"
    "\t\t\"tau_refrac\": 0.0,\n"
    "\t\t\"tau_m\": 50.0,\n"
    "\t\t\"cm\": 0.2\n"
    "\t},\n"
    "\t\"neuron_params2\": {\n"
    "\t\t\"e_rev_E\": 1.0,\n"
    "\t\t\"v_rest\": -65.0,\n"
    "\t\t\"v_reset\": -30.0,\n"
    "\t\t\"v_thresh\": -0.0,\n"
    "\t\t\"tau_syn_E\": 8.0,\n"
    "\t\t\"tau_refrac\": 16.0,\n"
    "\t\t\"tau_m\": 29.0,\n"
    "\t\t\"cm\": 300.0\n"
    "\t}\n"
    "}\n"
    "";

static const std::string overwrite_json =
    "{\n"
    "\t\"neuron_params\": {\n"
    "\t\t\"e_rev_E\": 8.0,\n"
    "\t\t\"v_rest\": 9.0,\n"
    "\t\t\"v_reset\": 10.0\n"
    "\t},\n"
    "\t\"neuron_params2\": {\n"
    "\t\t\"tau_syn_E\": 11.0,\n"
    "\t\t\"tau_refrac\": 12.0,\n"
    "\t\t\"tau_m\": 13.0,\n"
    "\t\t\"cm\": 14.0\n"
    "\t}\n"
    "}\n"
    "";
static const std::string sweep_json =
    "{\n"
    "\t\"neuron_params\": {\n"
    "\t\t\"e_rev_E\": [1,5,5],\n"
    "\t\t\"v_rest\": 9.0,\n"
    "\t\t\"v_reset\": 10.0\n"
    "\t},\n"
    "\t\"neuron_params2\": {\n"
    "\t\t\"tau_syn_E\": 11.0,\n"
    "\t\t\"tau_refrac\": 12.0,\n"
    "\t\t\"tau_m\": 13.0,\n"
    "\t\t\"cm\": 14.0\n"
    "\t}\n"
    "}\n"
    "";

static const std::string sweep2_json =
    "{\n"
    "\t\"neuron_params\": {\n"
    "\t\t\"e_rev_E\": [1,5,5],\n"
    "\t\t\"v_rest\": 9.0,\n"
    "\t\t\"v_reset\": 10.0\n"
    "\t},\n"
    "\t\"neuron_params2\": {\n"
    "\t\t\"tau_syn_E\": [1,3,3],\n"
    "\t\t\"tau_refrac\": 12.0,\n"
    "\t\t\"tau_m\": 13.0,\n"
    "\t\t\"cm\": 14.0\n"
    "\t}\n"
    "}\n"
    "";

TEST(generate_sweep_vector, overwrite)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	std::stringstream ss2(overwrite_json);
	cypress::Json json2 = cypress::Json::parse(ss2);
	std::vector<std::string> sweep_name;
	auto res = ParameterSweep::generate_sweep_vector(json2, json, sweep_name);
	EXPECT_EQ(size_t(1), res.size());
	EXPECT_EQ(size_t(0), sweep_name.size());
	EXPECT_NEAR(8.0, Real(res[0]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(9.0, Real(res[0]["neuron_params"]["v_rest"]), 1e-8);
	EXPECT_NEAR(10.0, Real(res[0]["neuron_params"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-57.0, Real(res[0]["neuron_params"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[0]["neuron_params"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(0.0, Real(res[0]["neuron_params"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(50.0, Real(res[0]["neuron_params"]["tau_m"]), 1e-8);
	EXPECT_NEAR(0.2, Real(res[0]["neuron_params"]["cm"]), 1e-8);

	EXPECT_NEAR(1.0, Real(res[0]["neuron_params2"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(-65.0, Real(res[0]["neuron_params2"]["v_rest"]), 1e-8);
	EXPECT_NEAR(-30.0, Real(res[0]["neuron_params2"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-0.0, Real(res[0]["neuron_params2"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(11.0, Real(res[0]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(12.0, Real(res[0]["neuron_params2"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(13.0, Real(res[0]["neuron_params2"]["tau_m"]), 1e-8);
	EXPECT_NEAR(14.0, Real(res[0]["neuron_params2"]["cm"]), 1e-8);
}

TEST(generate_sweep_vector, sweep)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	std::stringstream ss2(sweep_json);
	cypress::Json json2 = cypress::Json::parse(ss2);
	std::vector<std::string> sweep_name;
	auto res = ParameterSweep::generate_sweep_vector(json2, json, sweep_name);
	EXPECT_EQ(size_t(5), res.size());
	EXPECT_EQ(size_t(1), sweep_name.size());

	EXPECT_NEAR(1.0, Real(res[0]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(9.0, Real(res[0]["neuron_params"]["v_rest"]), 1e-8);
	EXPECT_NEAR(10.0, Real(res[0]["neuron_params"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-57.0, Real(res[0]["neuron_params"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[0]["neuron_params"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(0.0, Real(res[0]["neuron_params"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(50.0, Real(res[0]["neuron_params"]["tau_m"]), 1e-8);
	EXPECT_NEAR(0.2, Real(res[0]["neuron_params"]["cm"]), 1e-8);

	EXPECT_NEAR(1.0, Real(res[0]["neuron_params2"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(-65.0, Real(res[0]["neuron_params2"]["v_rest"]), 1e-8);
	EXPECT_NEAR(-30.0, Real(res[0]["neuron_params2"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-0.0, Real(res[0]["neuron_params2"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(11.0, Real(res[0]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(12.0, Real(res[0]["neuron_params2"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(13.0, Real(res[0]["neuron_params2"]["tau_m"]), 1e-8);
	EXPECT_NEAR(14.0, Real(res[0]["neuron_params2"]["cm"]), 1e-8);

	EXPECT_NEAR(2.0, Real(res[1]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(9.0, Real(res[1]["neuron_params"]["v_rest"]), 1e-8);
	EXPECT_NEAR(10.0, Real(res[1]["neuron_params"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-57.0, Real(res[1]["neuron_params"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[1]["neuron_params"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(0.0, Real(res[1]["neuron_params"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(50.0, Real(res[1]["neuron_params"]["tau_m"]), 1e-8);
	EXPECT_NEAR(0.2, Real(res[1]["neuron_params"]["cm"]), 1e-8);

	EXPECT_NEAR(1.0, Real(res[1]["neuron_params2"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(-65.0, Real(res[1]["neuron_params2"]["v_rest"]), 1e-8);
	EXPECT_NEAR(-30.0, Real(res[1]["neuron_params2"]["v_reset"]), 1e-8);
	EXPECT_NEAR(-0.0, Real(res[1]["neuron_params2"]["v_thresh"]), 1e-8);
	EXPECT_NEAR(11.0, Real(res[1]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(12.0, Real(res[1]["neuron_params2"]["tau_refrac"]), 1e-8);
	EXPECT_NEAR(13.0, Real(res[1]["neuron_params2"]["tau_m"]), 1e-8);
	EXPECT_NEAR(14.0, Real(res[1]["neuron_params2"]["cm"]), 1e-8);

	EXPECT_NEAR(3.0, Real(res[2]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(4.0, Real(res[3]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(5.0, Real(res[4]["neuron_params"]["e_rev_E"]), 1e-8);
}

TEST(generate_sweep_vector, 2dim_sweep)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	std::stringstream ss2(sweep2_json);
	cypress::Json json2 = cypress::Json::parse(ss2);
	std::vector<std::string> sweep_name;
	auto res = ParameterSweep::generate_sweep_vector(json2, json, sweep_name);
	EXPECT_EQ(size_t(15), res.size());

	EXPECT_NEAR(1.0, Real(res[0]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[1]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[2]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(4.0, Real(res[3]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(5.0, Real(res[4]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[5]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[6]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[7]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(4.0, Real(res[8]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(5.0, Real(res[9]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[10]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[11]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[12]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(4.0, Real(res[13]["neuron_params"]["e_rev_E"]), 1e-8);
	EXPECT_NEAR(5.0, Real(res[14]["neuron_params"]["e_rev_E"]), 1e-8);

	EXPECT_NEAR(1.0, Real(res[0]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[1]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[2]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[3]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(1.0, Real(res[4]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[5]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[6]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[7]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[8]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(2.0, Real(res[9]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[10]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[11]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[12]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[13]["neuron_params2"]["tau_syn_E"]), 1e-8);
	EXPECT_NEAR(3.0, Real(res[14]["neuron_params2"]["tau_syn_E"]), 1e-8);
}
}
