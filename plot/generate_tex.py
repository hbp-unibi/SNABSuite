#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   SNABSuite -- Spiking Neural Architecture Benchmark Suite
#   Copyright (C) 2017 Christoph Jenzen
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>


import json
import os

from dim_labels import *

# dictionary storing all results
results_dict = dict()
# Storing all variables which are arrays
config_array_dict = dict()


def result_json_to_global_dict(filename):
    """
    Adds results from file to global storage

    filename -- name of the file which will be used
    """

    with open(filename) as temp:
        json_temp = json.load(temp)

    simulator = filename.split(
        "/")[-1].split(".json")[0].split("_")[0].split("=")[0].split("pynn.")[-1]

    # Trigger key errors for unknown simulators
    if simulator not in SIMULATOR_LABELS:
        return

    # Get the array number
    array_nr = filename.split("/")[-1].split(".json")[0].split("_")[-1]
    if not array_nr.isdigit():
        array_nr = "0"

    if simulator not in results_dict:
        results_dict[simulator] = dict()
    results_dict[simulator][array_nr] = json_temp


def assemble_global_dict():
    """
    Gathers all results from build directory
    """

    for filename in os.listdir("../build/"):
        # All jsons besides invisible files and backup data
        if filename.endswith(".json") and not filename.startswith("."):
            if not filename.endswith("_bak.json"):
                result_json_to_global_dict("../build/" + filename)


def get_key_arrays(dictionary):
    """
    Iterates through the dictionary, gathers all entries with lists

    dictionary -- dict to iteratively
    """

    temp_dict = dict()
    for i in dictionary:
        if type(dictionary[i]) == list:
            temp_dict[i] = dictionary[i]
        else:
            if type(dictionary[i]) == dict:
                a = get_key_arrays(dictionary[i])
                if len(a) != 0:
                    temp_dict[i] = a
    return temp_dict


def get_key_arrays_from_config():
    """
    Gathers all arrays in configs from config directory
    """

    for filename in os.listdir("../config/"):
        if filename.endswith(".json") and not filename.startswith("."):
            with open("../config/" + filename) as temp:
                json_temp = json.load(temp)
            config_array_dict[filename.split(
                ".")[0]] = get_key_arrays(json_temp)


def gather_array_names(snabname):
    """
    Get the array names for the title of the table
    """

    array_names = []
    for i in config_array_dict[snabname]:  # Simulators
        for j in config_array_dict[snabname][i]:
            # Check if this is already included
            if not j in array_names:
                array_names.append(j)
    return array_names


def gather_benchmark_measures(snabname):
    """
    Get the measure names for the title of the table
    """

    array_names = []
    for i in results_dict:
        try:
            temp = results_dict[i]["0"]
        except:
            continue
        for j in temp:
            if not j["name"] == snabname:
                continue
            for k in j["results"]:
                array_names.append(k["name"] + " in " + k["measures"])
        if len(array_names) > 0:
            return array_names
    return array_names


def write_array_params(texfile, snab, backend, param_names, index):
    """
    Write parameters of the benchmark run

    texfile -- open file descriptor to file
    snab -- name string of the snab
    backend -- backend string
    param_names -- list of array names created by gather_array_names
    index -- index of the current array entry
    """

    for i in param_names:
        if backend in config_array_dict[snab] and i in config_array_dict[snab][backend]:
            texfile.write(
                " & " + str(config_array_dict[snab][backend][i][int(index)]))
        else:
            texfile.write(" & ")


def write_results(texfile, backend, index, snab, measures, got_snab):
    """
    Write results of the benchmark run

    texfile -- open file descriptor to file
    snab -- name string of the snab
    backend -- backend string
    measures -- list of measure names created by gather_benchmark_measures
    index -- index of the current array entry
    got_snab -- boolean wether there was at least one entry
    """

    for i in results_dict[backend][index]:
        if i["name"] == snab:
            got_snab = True
            for j in measures:
                found = False
                for k in i["results"]:
                    if k["name"] == j.split(" in ")[0]:
                        texfile.write(" & " + '{:.2f}'.format(k["value"]))
                        found = True
                # SNAB exists but measure was not found?
                if not found:
                    texfile.write(" & ")
            texfile.write("\n")
    
    # Fill in gaps if there were no results 
    if not got_snab:
        for j in measures:
            texfile.write(" & ")
        texfile.write("\n")
    return got_snab


assemble_global_dict()
get_key_arrays_from_config()

if not os.path.exists("tables"):
    os.mkdir("tables")

for snabname in config_array_dict:  # iterate over all SNABs
    print "Constructing ", snabname +".tex"
    # Assemble header: Platform, all array names, benchmark indicators
    param_names = gather_array_names(snabname)
    measures = gather_benchmark_measures(snabname)
    texfile = open("tables/" + snabname + ".tex", 'w')
    texfile.write("\\begin{tabular}{r")
    for i in xrange(0, len(param_names)):
        texfile.write(" r")
    for i in xrange(0, len(measures)):
        texfile.write(" l")
    texfile.write("}\n\\toprule\n")
    texfile.write("Platform")
    for i in xrange(0, len(param_names)):
        texfile.write(" & " + param_names[i])
    for i in xrange(0, len(measures)):
        texfile.write(" & " + measures[i])

    texfile.write("\n\\midrule\n")
    
    # Write data
    for backend in results_dict:
        texfile.write(backend)
        got_snab = False
        for k in xrange(0, len(results_dict[backend])):
            write_array_params(texfile, snabname, backend, param_names, str(k))
            got_snab = write_results(texfile, backend, str(
                k), snabname, measures, got_snab)

    texfile.write("\\bottomrule\n\end{tabular}")

