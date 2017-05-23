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

#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "util/utilities.hpp"
namespace SNAB {
TEST(Utilities, split)
{
	std::string st = "test.tested.and.testified";
	EXPECT_EQ(Utilities::split(st, ',')[0], st);
	EXPECT_EQ(Utilities::split(st, '.')[0], "test");
	EXPECT_EQ(Utilities::split(st, '.')[1], "tested");
	EXPECT_EQ(Utilities::split(st, '.')[2], "and");
	EXPECT_EQ(Utilities::split(st, '.')[3], "testified");
}

TEST(Utilities, calculate_statistics) {
    std::vector<double> empty;
    double min, max, avg, std_dev;
    Utilities::calculate_statistics(empty, min, max, avg, std_dev);
    EXPECT_EQ(min, 0);
    EXPECT_EQ(max, 0);
    EXPECT_EQ(avg, 0);
    EXPECT_EQ(std_dev, 0);
    
    std::vector<double> first = {2.0, 2.0, 3.0, 3.0};
    Utilities::calculate_statistics(first, min, max, avg, std_dev);
    EXPECT_NEAR(min, 2.0, 1e-6);
    EXPECT_NEAR(max, 3.0, 1e-6);
    EXPECT_NEAR(avg, 2.5, 1e-6);
    EXPECT_NEAR(std_dev, std::sqrt(1.0/3.0), 1e-6);
    
    
    first = {0.0, 2.0, 1.0, 3.0};
    Utilities::calculate_statistics(first, min, max, avg, std_dev);
    EXPECT_NEAR(min, 0.0, 1e-6);
    EXPECT_NEAR(max, 3.0, 1e-6);
    EXPECT_NEAR(avg, 1.5, 1e-6);
    EXPECT_NEAR(std_dev, std::sqrt(5.0/3.0), 1e-6);
    
    first = {3.0};
    Utilities::calculate_statistics(first, min, max, avg, std_dev);
    EXPECT_NEAR(min, 3.0, 1e-6);
    EXPECT_NEAR(max, 3.0, 1e-6);
    EXPECT_NEAR(avg, 3.0, 1e-6);
    EXPECT_NEAR(std_dev, 0, 1e-6);
}
}
