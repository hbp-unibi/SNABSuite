/*
 *  SNABSuite -- Spiking Neural Architecture Benchmark Suite
 *  Copyright (C) 2018  Christoph Ostrau
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

#include "SNABs/wta_like.hpp"
#include "gtest/gtest.h"

namespace SNAB {
using cypress::Real;

TEST(SimpleWTA, calculate_WTA_metrics)
{
	std::vector<size_t> bins({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
	std::vector<Real> res = SimpleWTA::calculate_WTA_metrics(bins, bins, 1);
	EXPECT_TRUE(res[0] != res[0]);
	EXPECT_TRUE(res[1] != res[0]);
	EXPECT_TRUE(res[2] != res[0]);

	std::vector<size_t> bins2({3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3});
	res = SimpleWTA::calculate_WTA_metrics(bins, bins2, 1.0);
	EXPECT_NEAR(0.0, res[0], 1e-8);
	EXPECT_NEAR(0.0, res[1], 1e-8);
	EXPECT_NEAR(14.0, res[2], 1e-8);

	res = SimpleWTA::calculate_WTA_metrics(bins2, bins, 1.0);
	EXPECT_NEAR(0.0, res[0], 1e-8);
	EXPECT_NEAR(0.0, res[1], 1e-8);
	EXPECT_NEAR(14.0, res[2], 1e-8);

	res = SimpleWTA::calculate_WTA_metrics(bins2, bins2, 1);
	EXPECT_NEAR(0.0, res[0], 1e-8);
	EXPECT_NEAR(0.0, res[1], 1e-8);
	EXPECT_NEAR(14.0, res[2], 1e-8);

	bins = std::vector<size_t>({0, 8, 8, 8, 0, 0, 0, 0, 4, 4, 8, 8, 8, 8});
	bins2 = std::vector<size_t>({0, 0, 0, 0, 8, 8, 8, 8, 4, 4, 0, 0, 0, 0});
	res = SimpleWTA::calculate_WTA_metrics(bins, bins2, 1);
	EXPECT_NEAR(4.0, res[0], 1e-8);
	EXPECT_NEAR(4.0, res[1], 1e-8);
	EXPECT_NEAR(3.0, res[2], 1e-8);
	res = SimpleWTA::calculate_WTA_metrics(bins2, bins, 1);
	EXPECT_NEAR(4.0, res[0], 1e-8);
	EXPECT_NEAR(4.0, res[1], 1e-8);
	EXPECT_NEAR(3.0, res[2], 1e-8);

	bins = std::vector<size_t>(20, 8);
	bins2 = std::vector<size_t>(20, 0);
	res = SimpleWTA::calculate_WTA_metrics(bins2, bins, 1);
	EXPECT_NEAR(20.0, res[0], 1e-8);
	EXPECT_NEAR(0.0, res[1], 1e-8);
	EXPECT_NEAR(0.0, res[2], 1e-8);
	res = SimpleWTA::calculate_WTA_metrics(bins, bins2, 1);
	EXPECT_NEAR(20.0, res[0], 1e-8);
	EXPECT_NEAR(0.0, res[1], 1e-8);
	EXPECT_NEAR(0.0, res[2], 1e-8);
}
}  // namespace SNAB
