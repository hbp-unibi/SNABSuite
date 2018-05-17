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

#include "gtest/gtest.h"
#include "util/utilities.hpp"
namespace SNAB {
TEST(Utilities, split)
{
	std::string st = "test.tested.and.testified";
	EXPECT_STREQ(st.c_str(), Utilities::split(st, ',')[0].c_str());
	EXPECT_STREQ("test", Utilities::split(st, '.')[0].c_str());
	EXPECT_STREQ("tested", Utilities::split(st, '.')[1].c_str());
	EXPECT_STREQ("and", Utilities::split(st, '.')[2].c_str());
	EXPECT_STREQ("testified", Utilities::split(st, '.')[3].c_str());
}

TEST(Utilities, calculate_statistics)
{
	std::vector<double> empty;
	double min, max, avg, std_dev;
	Utilities::calculate_statistics(empty, min, max, avg, std_dev);
	EXPECT_EQ(0, min);
	EXPECT_EQ(0, max);
	EXPECT_EQ(0, avg);
	EXPECT_EQ(0, std_dev);

	std::vector<double> first = {2.0, 2.0, 3.0, 3.0};
	Utilities::calculate_statistics(first, min, max, avg, std_dev);
	EXPECT_NEAR(2.0, min, 1e-6);
	EXPECT_NEAR(3.0, max, 1e-6);
	EXPECT_NEAR(2.5, avg, 1e-6);
	EXPECT_NEAR(std::sqrt(1.0 / 3.0), std_dev, 1e-6);

	first = {0.0, 2.0, 1.0, 3.0};
	Utilities::calculate_statistics(first, min, max, avg, std_dev);
	EXPECT_NEAR(0.0, min, 1e-6);
	EXPECT_NEAR(3.0, max, 1e-6);
	EXPECT_NEAR(1.5, avg, 1e-6);
	EXPECT_NEAR(std::sqrt(5.0 / 3.0), std_dev, 1e-6);

	first = {3.0};
	Utilities::calculate_statistics(first, min, max, avg, std_dev);
	EXPECT_NEAR(3.0, min, 1e-6);
	EXPECT_NEAR(3.0, max, 1e-6);
	EXPECT_NEAR(3.0, avg, 1e-6);
	EXPECT_NEAR(0, std_dev, 1e-6);
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

static const std::string test_json5 =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"misc\": [21,22,23]\n"
    "\t}\n"
    "}\n";
static const std::string test_json6 =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"misc2\": [21,22,23]\n"
    "\t}\n"
    "}\n";

using cypress::Json;
TEST(Utilities, merge_json)
{
	Json json1 = Json::parse(test_json1);
	Json json2 = Json::parse(test_json2);
	Json json3 = Json::parse(test_json3);
	Json json4 = Json::parse(test_json4);
	Json json5 = Json::parse(test_json5);
	Json json6 = Json::parse(test_json6);

	// Test for overwriting values
	Json test1 = Utilities::merge_json(json1, json2);
	EXPECT_EQ(200, int(test1["data"]["n_bits_in"]));
	EXPECT_EQ(20, int(test1["data"]["n_bits_out"]));
	EXPECT_EQ(1, int(test1["data"]["n_ones_in"]));
	EXPECT_EQ(2, int(test1["data"]["n_ones_out"]));
	EXPECT_EQ(5, int(test1["data"]["n_samples"]));

	// Test for appending values
	Json test2 = Utilities::merge_json(json1, json3);
	EXPECT_EQ(200, int(test2["misc"]["n_bits_in"]));
	EXPECT_EQ(20, int(test2["misc"]["n_bits_out"]));
	EXPECT_EQ(1, int(test2["misc"]["n_ones_in"]));
	EXPECT_EQ(2, int(test2["misc"]["n_ones_out"]));
	EXPECT_EQ(5, int(test2["misc"]["n_samples"]));

	// Test for append in a sub structure
	Json test3 = Utilities::merge_json(json1, json4);
	EXPECT_EQ(21, int(test3["data"]["misc"]));

	// Test for replace entry by array
	Json test4 = Utilities::merge_json(json4, json5);
	EXPECT_EQ(json5, test4);
	Json test5 = Utilities::merge_json(json5, json6);
	EXPECT_EQ(json5["data"]["misc"], test5["data"]["misc"]);
	EXPECT_EQ(json6["data"]["misc2"], test5["data"]["misc2"]);
    
    // Test for replace array by entry
    Json test6 = Utilities::merge_json(json5, json4);
	EXPECT_EQ(json4, test6);
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
	EXPECT_STREQ("back", Utilities::split(backend1, '=')[0].c_str());
	EXPECT_EQ(200, int(test2["data"]["n_bits_in"]));
	EXPECT_EQ(20, int(test2["data"]["n_bits_out"]));
	EXPECT_EQ(1, int(test2["data"]["n_ones_in"]));
	EXPECT_EQ(2, int(test2["data"]["n_ones_out"]));
	EXPECT_EQ(5, int(test2["data"]["n_samples"]));

	Utilities::manipulate_backend_string(backend2, json2);
	test2 = Json::parse(Utilities::split(backend2, '=')[1]);
	EXPECT_STREQ("back", Utilities::split(backend2, '=')[0].c_str());
	EXPECT_EQ(200, int(test2["data"]["n_bits_in"]));
	EXPECT_EQ(20, int(test2["data"]["n_bits_out"]));
	EXPECT_EQ(1, int(test2["data"]["n_ones_in"]));
	EXPECT_EQ(2, int(test2["data"]["n_ones_out"]));
	EXPECT_EQ(5, int(test2["data"]["n_samples"]));
	EXPECT_EQ(3, int(test2["bla"]));

	Utilities::manipulate_backend_string(backend3, json2);
	test2 = Json::parse(Utilities::split(backend3, '=')[1]);
	EXPECT_STREQ("back", Utilities::split(backend3, '=')[0].c_str());
	EXPECT_EQ(200, int(test2["data"]["n_bits_in"]));
	EXPECT_EQ(20, int(test2["data"]["n_bits_out"]));
	EXPECT_EQ(1, int(test2["data"]["n_ones_in"]));
	EXPECT_EQ(2, int(test2["data"]["n_ones_out"]));
	EXPECT_EQ(5, int(test2["data"]["n_samples"]));
	EXPECT_EQ(18, int(test2["data"]["misc"]));

	backend3 = "back={\"data\" : {\"misc\": 18 }}";
	Utilities::manipulate_backend_string(backend3, json4);
	test2 = Json::parse(Utilities::split(backend3, '=')[1]);
	EXPECT_STREQ("back", Utilities::split(backend3, '=')[0].c_str());
	EXPECT_EQ(18, int(test2["data"]["misc"]));
}
}  // namespace SNAB
