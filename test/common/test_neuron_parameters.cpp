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

#include <sstream>
#include <vector>

#include "gtest/gtest.h"

#include <cypress/cypress.hpp>
#include "common/neuron_parameters.hpp"

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
    "\t}\n"
    "}\n"
    "";

TEST(NeuronParameters, NeuronParameters)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	const cypress::IfCondExp &neurontype = cypress::IfCondExp::inst();
	auto params = NeuronParameters(neurontype, json["neuron_params"]);
	std::vector<cypress::Real> parameter = params.parameter();
	EXPECT_NEAR(Real(0.2), parameter[0], 1e-8);
	EXPECT_NEAR(Real(50), parameter[1], 1e-8);
	EXPECT_NEAR(Real(2), parameter[2], 1e-8);
	EXPECT_NEAR(Real(5), parameter[3], 1e-8);  // Cypress standard
	EXPECT_NEAR(Real(0.0), parameter[4], 1e-8);
	EXPECT_NEAR(Real(-70), parameter[5], 1e-8);
	EXPECT_NEAR(Real(-57), parameter[6], 1e-8);
	EXPECT_NEAR(Real(-80), parameter[7], 1e-8);
	EXPECT_NEAR(Real(0), parameter[8], 1e-8);
	EXPECT_NEAR(Real(-70), parameter[9], 1e-8);  // Cypress standard
	EXPECT_NEAR(Real(0), parameter[10], 1e-8);   // Cypress standard
}

TEST(NeuronParameters, set)
{
	cypress::Json json;
	const cypress::IfCondExp &neurontype = cypress::IfCondExp::inst();
	std::ofstream out;
	auto params = NeuronParameters(neurontype, json);
	EXPECT_NEAR(Real(0.53), params.set("cm", 0.53).parameter()[0], 1e-8);
	EXPECT_NEAR(Real(0.43), params.set("tau_m", 0.43).parameter()[1], 1e-8);
	EXPECT_NEAR(Real(0.33), params.set("tau_syn_E", 0.33).parameter()[2], 1e-8);
	EXPECT_NEAR(Real(0.23), params.set("tau_syn_I", 0.23).parameter()[3], 1e-8);
	EXPECT_NEAR(Real(0.13), params.set("tau_refrac", 0.13).parameter()[4],
	            1e-8);
	EXPECT_NEAR(Real(0.03), params.set("v_rest", 0.03).parameter()[5], 1e-8);
	EXPECT_NEAR(Real(0.63), params.set("v_thresh", 0.63).parameter()[6], 1e-8);
	EXPECT_NEAR(Real(0.73), params.set("v_reset", 0.73).parameter()[7], 1e-8);
	EXPECT_NEAR(Real(0.83), params.set("e_rev_E", 0.83).parameter()[8], 1e-8);
	EXPECT_NEAR(Real(0.93), params.set("e_rev_I", 0.93).parameter()[9], 1e-8);
	EXPECT_NEAR(Real(1.03), params.set("i_offset", 1.03).parameter()[10], 1e-8);
}

TEST(NeuronParameters, get)
{
	cypress::Json json;
	const cypress::IfCondExp &neurontype = cypress::IfCondExp::inst();
	std::ofstream out;
	auto params = NeuronParameters(neurontype, json);
	EXPECT_NEAR(Real(1.0), params.get("cm"), 1e-8);
	EXPECT_NEAR(Real(20.0), params.get("tau_m"), 1e-8);
	EXPECT_NEAR(Real(5.0), params.get("tau_syn_E"), 1e-8);
	EXPECT_NEAR(Real(5.0), params.get("tau_syn_I"), 1e-8);
	EXPECT_NEAR(Real(0.1), params.get("tau_refrac"), 1e-8);
	EXPECT_NEAR(Real(-65.0), params.get("v_rest"), 1e-8);
	EXPECT_NEAR(Real(-50.0), params.get("v_thresh"), 1e-8);
	EXPECT_NEAR(Real(-65.0), params.get("v_reset"), 1e-8);
	EXPECT_NEAR(Real(0.0), params.get("e_rev_E"), 1e-8);
	EXPECT_NEAR(Real(-70.0), params.get("e_rev_I"), 1e-8);
	EXPECT_NEAR(Real(0.0), params.get("i_offset"), 1e-8);
}
}
