#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   SNABSuite -- Spiking Neural Architecture Benchmark Suite
#   Copyright (C) 2019 Christoph Ostrau
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
Plots data from one dimensional sweeps
"""
from builtins import range
import argparse
import json
import glob
import numpy

parser = argparse.ArgumentParser(description='Autoformat json files in a folder')

# Required Parameters
parser.add_argument("files", metavar="files", type=str, help="path to folder")
args = parser.parse_args()

list_files = glob.glob(args.files + "*.json")

# Adapted from https://stackoverflow.com/questions/21866774/pretty-print-json-dumps
INDENT = 4
SPACE = " "
NEWLINE = "\n"

def to_json(o, level=0):
    ret = ""
    if isinstance(o, dict):
        ret += "{" + NEWLINE
        comma = ""
        for k,v in o.items():
            ret += comma
            comma = ",\n"
            ret += SPACE * INDENT * (level+1)
            ret += '"' + str(k) + '":' + SPACE
            ret += to_json(v, level + 1)

        ret += NEWLINE + SPACE * INDENT * level + "}"
    elif isinstance(o, str):
        ret += '"' + o + '"'
    elif isinstance(o, list):
        ret += "[" + ",".join([to_json(e, level+1) for e in o]) + "]"
    elif isinstance(o, bool):
        ret += "true" if o else "false"
    elif isinstance(o, int):
        ret += str(o)
    elif isinstance(o, float):
        ret += '%.7g' % o
    elif isinstance(o, numpy.ndarray) and numpy.issubdtype(o.dtype, numpy.integer):
        ret += "[" + ','.join(map(str, o.flatten().tolist())) + "]"
    elif isinstance(o, numpy.ndarray) and numpy.issubdtype(o.dtype, numpy.inexact):
        ret += "[" + ','.join(map(lambda x: '%.7g' % x, o.flatten().tolist())) + "]"
    else:
        raise TypeError("Unknown type '%s' for json serialization" % str(type(o)))
    return ret

def prettyfy(filename):
    with open(filename) as file:
        data = json.load(file)
    with open(filename, 'w') as file:
        file.write(to_json(data))
    
for i in list_files:
    print(i)
    prettyfy(i)
