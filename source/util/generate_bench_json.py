#!/usr/bin/python
# -*- coding: utf-8 -*-

#   SNABSuite -- Spiking Neural Architecture Benchmark Suite
#   Copyright (C) 2018 Christoph Ostrau
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

"""
Generate benchmarks.json for the web frontend 
@author Michael Thies, Christoph Ostrau
"""
import sys
import CppHeaderParser
import re
import os
import json

simulators = ["nest", "spinnaker", "nmpm1", "spikey"]  # SNABSuite simulators
web_simulators = ["nest", "spiNNaker", "hardware.hbp_pm",
                  "spikey"]  # Web frontend simulators
bench_index = ["Single Core/Smallest Network", "Single Chip",
               "Small System", "Large System"]  # Benchmark index and their description


def get_key_arrays(dictionary):
    """
    Iterates through the dictionary, gathers all entries with lists

    dictionary -- dict to iteratively search for lists
    """

    temp_dict = dict()
    length = 0
    for i in dictionary:
        if type(dictionary[i]) == list:
            temp_dict[i] = dictionary[i]
            if(length < len(dictionary[i])):
                length = len(dictionary[i])
        else:
            if type(dictionary[i]) == dict:
                a, length2 = get_key_arrays(dictionary[i])
                if len(a) != 0:
                    temp_dict[i] = a
                if(length < length2):
                    length = length2
    return temp_dict, length


def get_available_simulators(dictionary):
    """
    Checks the dictionary for lists of every simulator to gather information 
    about which simulator is available at which network size

    dictionary -- config file to search for dictionaries
    """
    avail_list = []
    global_length = 0
    for simulator in simulators:
        try:
            a, length = get_key_arrays(dictionary[simulator])
            if length > global_length:
                global_length = length
            avail_list.append(length)
        except:
            avail_list.append(0)

    if global_length == 0:  # Not a scalable benchmark
        avail_list = [1 for i in avail_list]

    # Check for invalid flag
    for i, simulator in enumerate(simulators):
        if "invalid" in dictionary[simulator]:
            if dictionary[simulator]["invalid"]:
                avail_list[i] = 0

    return avail_list


global_json = []  # This will be written to file
for headername in os.listdir("../source/SNABs/"):
    if not headername.endswith(".hpp"):
        continue  # Check if it is a header
    try:
        cppHeader = CppHeaderParser.CppHeader("../source/SNABs/" + headername)
    except CppHeaderParser.CppParseError as e:
        print(e)
        sys.exit(1)  # Exit with error so that travis will report an issue

    #print("CppHeaderParser view of %s"%cppHeader)

    print headername
    for classname in cppHeader.classes:
        # Every class represents a SNAB/benchmark
        clazz = cppHeader.classes[classname]
        print 'class', clazz['name']
        comment = ""
        if 'doxygen' in clazz:
            comment = clazz['doxygen']
            # strip start of comment marker
            comment = re.sub(r'^/\*+\s*\n', '', comment)
            # strip end of comment marker
            comment = re.sub(r'\n\s*\*/', '', comment)
            # strip leading * at start of each line in comment
            comment = re.sub(r'^\s*\*\s*', '', comment, 0, re.MULTILINE)
            # unwrap the text into a single line
            comment = comment.replace('\n', ' ')

        # One dict per SNAB
        snab_dict = {}
        snab_dict["model"] = {"name": clazz['name'],
                              "description": comment}

        # Open corresponding config file to generate tasks
        with open("../config/" + clazz['name'] + ".json") as temp:
            json_temp = json.load(temp)
        json_temp2, length = get_key_arrays(json_temp)
        avail_list = get_available_simulators(json_temp)

        tasks = []  # Generate task list
        if length == 0:  # Special case, not a scalable benchmark
            length = 1
        set_snab = False  # Is there any viable benchmark
        for i in xrange(0, length):
            set_task = False  # Is there any viable task
            task_dict = {}
            task_dict["name"] = bench_index[i]
            task_dict["command"] = "./benchmark " " {system="
            for j in xrange(0, len(simulators)):
                if i < avail_list[j]:
                    task_dict["command"] += web_simulators[j] + ","
                    set_task = True
            if not set_task:
                continue
            task_dict["command"] = task_dict["command"][:-1]
            task_dict["command"] += "} " + clazz['name'] + " " + str(i)
            tasks.append(task_dict)
            set_snab = True
        if not set_snab:
            continue

        snab_dict["tasks"] = tasks
        global_json.append(snab_dict)
        print '--------'
    print '========'

# Dump the global list of benchmark description in pretty
with open('benchmarks.json', 'w') as file:
    json.dump(global_json, file, indent=4, sort_keys=True)
