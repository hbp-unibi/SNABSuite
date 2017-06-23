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

TEST(Utilities, calculate_statistics)
{
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
	EXPECT_NEAR(std_dev, std::sqrt(1.0 / 3.0), 1e-6);

	first = {0.0, 2.0, 1.0, 3.0};
	Utilities::calculate_statistics(first, min, max, avg, std_dev);
	EXPECT_NEAR(min, 0.0, 1e-6);
	EXPECT_NEAR(max, 3.0, 1e-6);
	EXPECT_NEAR(avg, 1.5, 1e-6);
	EXPECT_NEAR(std_dev, std::sqrt(5.0 / 3.0), 1e-6);

	first = {3.0};
	Utilities::calculate_statistics(first, min, max, avg, std_dev);
	EXPECT_NEAR(min, 3.0, 1e-6);
	EXPECT_NEAR(max, 3.0, 1e-6);
	EXPECT_NEAR(avg, 3.0, 1e-6);
	EXPECT_NEAR(std_dev, 0, 1e-6);
}

static const std::string test_json1 =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"n_bits_in\": 100,\n"
    "\t\t\"n_bits_out\": 100,\n"
    "\t\t\"n_ones_in\": 4,\n"
    "\t\t\"n_ones_out\": 4,\n"
    "\t\t\"n_samples\" : 1000\n"
    "\t},\n"
    "\n"
    "\t\"network\": {\n"
    "\t\t\"params\": {\n"
    "\t\t\t\"e_rev_E\": 0.0,\n"
    "\t\t\t\"v_rest\": -70.0,\n"
    "\t\t\t\"v_reset\": -80.0,\n"
    "\t\t\t\"v_thresh\": -57.0,\n"
    "\t\t\t\"tau_syn_E\": 2.0,\n"
    "\t\t\t\"tau_refrac\": 0.0,\n"
    "\t\t\t\"tau_m\": 50.0,\n"
    "\t\t\t\"cm\": 0.2\n"
    "\t\t},\n"
    "\t\t\"neuron_type\": \"IF_cond_exp\",\n"
    "\t\t\"weight\": 0.01,\n"
    "\t\t\"input_burst_size\": 1,\n"
    "\t\t\"output_burst_size\": 1,\n"
    "\t\t\"time_window\": 100.0,\n"
    "\t\t\"isi\": 2.0,\n"
    "\t\t\"sigma_t\": 2.0,\n"
    "\t\t\"sigma_offs\": 0.0,\n"
    "\t\t\"p0\": 0.0,\n"
    "\t\t\"p1\": 0.0,\n"
    "\t\t\"general_offset\" : 100\n"
    "\t}\n"
    "}\n"
    "";

static const std::string test_json2 =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"n_bits_in\": 200,\n"
    "\t\t\"n_bits_out\": 20,\n"
    "\t\t\"n_ones_in\": 1,\n"
    "\t\t\"n_ones_out\": 2,\n"
    "\t\t\"n_samples\" : 5\n"
    "\t}\n"
    "}\n";

static const std::string test_json3 =
    "{\n"
    "\t\"misc\": {\n"
    "\t\t\"n_bits_in\": 200,\n"
    "\t\t\"n_bits_out\": 20,\n"
    "\t\t\"n_ones_in\": 1,\n"
    "\t\t\"n_ones_out\": 2,\n"
    "\t\t\"n_samples\" : 5\n"
    "\t}\n"
    "}\n";

static const std::string test_json4 =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"misc\": 21\n"
    "\t}\n"
    "}\n";

using cypress::Json;
TEST(Utilities, merge_json)
{
	Json json1 = Json::parse(test_json1);
	Json json2 = Json::parse(test_json2);
	Json json3 = Json::parse(test_json3);
	Json json4 = Json::parse(test_json4);

	// Test for overwriting values
	Json test1 = Utilities::merge_json(json1, json2);
	EXPECT_EQ(test1["data"]["n_bits_in"], 200);
	EXPECT_EQ(test1["data"]["n_bits_out"], 20);
	EXPECT_EQ(test1["data"]["n_ones_in"], 1);
	EXPECT_EQ(test1["data"]["n_ones_out"], 2);
	EXPECT_EQ(test1["data"]["n_samples"], 5);

	// Test for appending values
	Json test2 = Utilities::merge_json(json1, json3);
	EXPECT_EQ(test2["misc"]["n_bits_in"], 200);
	EXPECT_EQ(test2["misc"]["n_bits_out"], 20);
	EXPECT_EQ(test2["misc"]["n_ones_in"], 1);
	EXPECT_EQ(test2["misc"]["n_ones_out"], 2);
	EXPECT_EQ(test2["misc"]["n_samples"], 5);

	// Test for append in a sub structure
	Json test3 = Utilities::merge_json(json1, json4);
	EXPECT_EQ(test3["data"]["misc"], 21);
}

TEST(Utilities, manipulate_backend)
{
	Json json2 = Json::parse(test_json2);
	Json json3 = Json::parse(test_json3);
	Json json4 = Json::parse(test_json4);
	std::string backend1 = "back";
	std::string backend2 = "back={\"bla\": 3}";
	std::string backend3 = "back={\"data\" : {\"misc\": 18 }}";

	Utilities::manipulate_backend_string(backend1, json2);
	Json test2 = Json::parse(Utilities::split(backend1, '=')[1]);
	EXPECT_EQ("back", Utilities::split(backend1, '=')[0]);
	EXPECT_EQ(200, test2["data"]["n_bits_in"]);
	EXPECT_EQ(20, test2["data"]["n_bits_out"]);
	EXPECT_EQ(1, test2["data"]["n_ones_in"]);
	EXPECT_EQ(2, test2["data"]["n_ones_out"]);
	EXPECT_EQ(5, test2["data"]["n_samples"]);

	Utilities::manipulate_backend_string(backend2, json2);
	test2 = Json::parse(Utilities::split(backend2, '=')[1]);
	EXPECT_EQ("back", Utilities::split(backend2, '=')[0]);
	EXPECT_EQ(200, test2["data"]["n_bits_in"]);
	EXPECT_EQ(20, test2["data"]["n_bits_out"]);
	EXPECT_EQ(1, test2["data"]["n_ones_in"]);
	EXPECT_EQ(2, test2["data"]["n_ones_out"]);
	EXPECT_EQ(5, test2["data"]["n_samples"]);
	EXPECT_EQ(3, test2["bla"]);

	Utilities::manipulate_backend_string(backend3, json2);
	test2 = Json::parse(Utilities::split(backend3, '=')[1]);
	EXPECT_EQ("back", Utilities::split(backend3, '=')[0]);
	EXPECT_EQ(200, test2["data"]["n_bits_in"]);
	EXPECT_EQ(20, test2["data"]["n_bits_out"]);
	EXPECT_EQ(1, test2["data"]["n_ones_in"]);
	EXPECT_EQ(2, test2["data"]["n_ones_out"]);
	EXPECT_EQ(5, test2["data"]["n_samples"]);
	EXPECT_EQ(18, test2["data"]["misc"]);

	backend3 = "back={\"data\" : {\"misc\": 18 }}";
	Utilities::manipulate_backend_string(backend3, json4);
	test2 = Json::parse(Utilities::split(backend3, '=')[1]);
	EXPECT_EQ("back", Utilities::split(backend3, '=')[0]);
	EXPECT_EQ(18, test2["data"]["misc"]);
}
}
