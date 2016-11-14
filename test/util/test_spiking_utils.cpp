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

#include <string>

#include <cypress/cypress.hpp>

#include <gtest/gtest.h>

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
	SpikingUtils::add_population(type, netw, params, 1);
	EXPECT_EQ(netw.population_count(), 1);
	EXPECT_EQ(netw.neuron_count(), 1);
	EXPECT_EQ(netw.neuron_count(IfCondExp::inst()), 1);
	EXPECT_EQ(netw.neuron_count(IfFacetsHardware1::inst()), 0);

	auto pop = netw.population();
	EXPECT_EQ(pop.size(), 1);
	EXPECT_EQ(&pop.type(), &IfCondExp::inst());
}
}
