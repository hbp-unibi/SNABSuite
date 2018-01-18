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

#include "gtest/gtest.h"

#include "common/neuron_parameters.hpp"
#include "util/read_json.hpp"

namespace SNAB {

static const std::string test_json =
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

TEST(ReadJSON, json_to_map)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);

	auto map = json_to_map<float>(json["network"]);
	EXPECT_EQ(map.end(), map.find("neuron_type"));
	EXPECT_NEAR(0.01, map.find("weight")->second, 1e-8);
	EXPECT_NEAR(1.0, map.find("input_burst_size")->second, 1e-8);
	EXPECT_NEAR(1.0, map.find("output_burst_size")->second, 1e-8);
	EXPECT_NEAR(100.0, map.find("time_window")->second, 1e-8);
	EXPECT_NEAR(2.0, map.find("isi")->second, 1e-8);
	EXPECT_NEAR(2.0, map.find("sigma_t")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("sigma_offs")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("p0")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("p1")->second, 1e-8);
	EXPECT_NEAR(100.0, map.find("general_offset")->second, 1e-8);
}

TEST(ReadJSON, read_check)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);

	auto map = json_to_map<float>(json["network"]["params"]);
	std::vector<std::string> names = {"e_rev_E",  "v_rest",    "v_reset",
	                                  "v_thresh", "tau_syn_E", "tau_refrac",
	                                  "tau_m",    "cm"};

	std::vector<float> def = {1, 1, 1, 1, 1, 1, 1, 1};

	auto res = read_check<float>(map, names, def);
	EXPECT_NEAR(res[0], 0.0, 1e-8);
	EXPECT_NEAR(res[1], -70.0, 1e-8);
	EXPECT_NEAR(res[2], -80.0, 1e-8);
	EXPECT_NEAR(res[3], -57.0, 1e-8);
	EXPECT_NEAR(res[4], 2.0, 1e-8);
	EXPECT_NEAR(res[5], 0.0, 1e-8);
	EXPECT_NEAR(res[6], 50.0, 1e-8);
	EXPECT_NEAR(res[7], 0.2, 1e-8);

	map = json_to_map<float>(json["network"]);
	EXPECT_ANY_THROW(
	    read_check<float>(map, std::vector<std::string>({"input_burst_size"}),
	                      std::vector<float>({0})));
}

TEST(ReadJSON, read_config)
{
	EXPECT_FALSE(read_config("bla", "blup")["valid"]);
	EXPECT_NO_THROW(read_config("OutputFrequencySingleNeuron", "spinnaker"));
	auto config = read_config("OutputFrequencySingleNeuron",
	                          "spinnaker")["neuron_params"];
	EXPECT_NEAR(config["e_rev_E"], 0.0, 1e-8);
	EXPECT_NEAR(config["v_rest"], -30.0, 1e-8);
	EXPECT_NEAR(config["v_reset"], -60.0, 1e-8);
	EXPECT_NEAR(config["v_thresh"], -64.7, 1e-8);
	EXPECT_NEAR(config["tau_syn_E"], 2.0, 1e-8);
	EXPECT_NEAR(config["tau_refrac"], 0.0, 1e-8);
	EXPECT_NEAR(config["tau_m"], 10.0, 1e-8);
	EXPECT_NEAR(config["cm"], 0.2, 1e-8);

	EXPECT_NO_THROW(read_config("OutputFrequencySingleNeuron", "spikey"));
}

TEST(ReadJSON, check_json_for_parameters)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	std::vector<std::string> names({"data", "network"});
	EXPECT_TRUE(check_json_for_parameters(names, json, "bla"));
	std::vector<std::string> names2(
	    {"n_bits_in", "n_bits_out", "n_ones_in", "n_ones_out", "n_samples"});
	EXPECT_TRUE(check_json_for_parameters(names2, json["data"], "bla"));
	names2.emplace_back("wrong_test");
	EXPECT_FALSE(check_json_for_parameters(names, json["data"], "bla"));
}

static const cypress::Json json_array = {0, 5, 8, 9, 4, 2, 9, 1, 0, 5, 33, 27};
static const cypress::Json json_array2 = {0.3, 5.2, 8.8,  9.1, 4.4,  2.8,
                                          9.9, 1.4, 0.22, 5.3, 33.0, 27.5555};
static const cypress::Json json_2Darray = {
    {0, 5, 8}, {9, 4, 2}, {9, 1, 0}, {5, 33, 27}};
static const cypress::Json json_2Darray2 = {
    {0.3, 5.2, 8.8}, {9.1, 4.4, 2.8}, {9.9, 1.4, 0.22}, {5.3, 33.0, 27.5555}};
TEST(ReadJSON, json_array_to_vector)
{
	std::vector<int> vec = json_array_to_vector<int>(json_array);
	EXPECT_EQ(0, vec[0]);
	EXPECT_EQ(5, vec[1]);
	EXPECT_EQ(8, vec[2]);
	EXPECT_EQ(9, vec[3]);
	EXPECT_EQ(4, vec[4]);
	EXPECT_EQ(2, vec[5]);
	EXPECT_EQ(9, vec[6]);
	EXPECT_EQ(1, vec[7]);
	EXPECT_EQ(0, vec[8]);
	EXPECT_EQ(5, vec[9]);
	EXPECT_EQ(33, vec[10]);
	EXPECT_EQ(27, vec[11]);

	std::vector<double> vec2 = json_array_to_vector<double>(json_array2);
	EXPECT_NEAR(0.3, vec2[0], 1e-8);
	EXPECT_NEAR(5.2, vec2[1], 1e-8);
	EXPECT_NEAR(8.8, vec2[2], 1e-8);
	EXPECT_NEAR(9.1, vec2[3], 1e-8);
	EXPECT_NEAR(4.4, vec2[4], 1e-8);
	EXPECT_NEAR(2.8, vec2[5], 1e-8);
	EXPECT_NEAR(9.9, vec2[6], 1e-8);
	EXPECT_NEAR(1.4, vec2[7], 1e-8);
	EXPECT_NEAR(0.22, vec2[8], 1e-8);
	EXPECT_NEAR(5.3, vec2[9], 1e-8);
	EXPECT_NEAR(33.0, vec2[10], 1e-8);
	EXPECT_NEAR(27.5555, vec2[11], 1e-8);

	ASSERT_ANY_THROW(json_array_to_vector<double>(cypress::Json({{"foo", 3}})));
	ASSERT_ANY_THROW(json_array_to_vector<double>(json_2Darray));
	ASSERT_ANY_THROW(json_array_to_vector<double>(json_2Darray2));
}

TEST(ReadJSON, json_2Darray_to_vector)
{
	std::vector<std::vector<int>> vec =
	    json_2Darray_to_vector<int>(json_2Darray);
	EXPECT_EQ(0, vec[0][0]);
	EXPECT_EQ(5, vec[0][1]);
	EXPECT_EQ(8, vec[0][2]);
	EXPECT_EQ(9, vec[1][0]);
	EXPECT_EQ(4, vec[1][1]);
	EXPECT_EQ(2, vec[1][2]);
	EXPECT_EQ(9, vec[2][0]);
	EXPECT_EQ(1, vec[2][1]);
	EXPECT_EQ(0, vec[2][2]);
	EXPECT_EQ(5, vec[3][0]);
	EXPECT_EQ(33, vec[3][1]);
	EXPECT_EQ(27, vec[3][2]);

	std::vector<std::vector<double>> vec2 =
	    json_2Darray_to_vector<double>(json_2Darray2);
	EXPECT_NEAR(0.3, vec2[0][0], 1e-8);
	EXPECT_NEAR(5.2, vec2[0][1], 1e-8);
	EXPECT_NEAR(8.8, vec2[0][2], 1e-8);
	EXPECT_NEAR(9.1, vec2[1][0], 1e-8);
	EXPECT_NEAR(4.4, vec2[1][1], 1e-8);
	EXPECT_NEAR(2.8, vec2[1][2], 1e-8);
	EXPECT_NEAR(9.9, vec2[2][0], 1e-8);
	EXPECT_NEAR(1.4, vec2[2][1], 1e-8);
	EXPECT_NEAR(0.22, vec2[2][2], 1e-8);
	EXPECT_NEAR(5.3, vec2[3][0], 1e-8);
	EXPECT_NEAR(33.0, vec2[3][1], 1e-8);
	EXPECT_NEAR(27.5555, vec2[3][2], 1e-8);

	ASSERT_ANY_THROW(
	    json_2Darray_to_vector<double>(cypress::Json({{"foo", 3}})));
	ASSERT_ANY_THROW(json_2Darray_to_vector<double>(json_array));
	ASSERT_ANY_THROW(json_2Darray_to_vector<double>(json_array2));
}

TEST(ReadJSON, replace_arrays_by_value)
{
	std::stringstream ss(test_json);
	cypress::Json json = cypress::Json::parse(ss);
	cypress::Json json2 = json;
	bool changed = false;

	// Check that nothing is changed if there is no array
	changed = replace_arrays_by_value(json2);
	EXPECT_FALSE(changed);
	EXPECT_EQ(json, json2);
	changed = replace_arrays_by_value(json2, 2);
	EXPECT_FALSE(changed);
	EXPECT_EQ(json, json2);
	changed = replace_arrays_by_value(json2, 3);
	EXPECT_FALSE(changed);
	EXPECT_EQ(json, json2);

	json2["data"]["n_bits_out"] = {100, 200, 300, 400};
	changed = replace_arrays_by_value(json2);
	EXPECT_TRUE(changed);
	EXPECT_EQ(json, json2);
	json2["data"]["n_bits_out"] = {100, 200, 300, 400};
	changed = replace_arrays_by_value(json2, 1);
	EXPECT_TRUE(changed);
	EXPECT_NE(json, json2);
	EXPECT_EQ(200, int(json2["data"]["n_bits_out"]));

	json2["data"]["n_bits_out"] = {100, 200, 300, 400};
	changed = replace_arrays_by_value(json2, 2);
	EXPECT_TRUE(changed);
	EXPECT_EQ(300, int(json2["data"]["n_bits_out"]));

	json2["new_key"] = {1, 3, 5, 28};
	EXPECT_EQ(cypress::Json({1, 3, 5, 28}), json2["new_key"]);
	changed = replace_arrays_by_value(json2);
	EXPECT_TRUE(changed);
	EXPECT_EQ(1, int(json2["new_key"]));

	json2["new_key"] = {1, 3, 5, 28};
	changed = replace_arrays_by_value(json2, 1);
	EXPECT_TRUE(changed);
	EXPECT_EQ(3, int(json2["new_key"]));

	json2["new_key"] = {1, 3, 5, 28};
	changed = replace_arrays_by_value(json2, 3);
	EXPECT_TRUE(changed);
	EXPECT_EQ(28, int(json2["new_key"]));

	json2["new_key"] = {1, 3, 5, 28};
	ASSERT_NO_THROW(replace_arrays_by_value(json2, 4));
	EXPECT_FALSE(replace_arrays_by_value(json2, 4));
}
}
