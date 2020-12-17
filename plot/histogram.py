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

"""
Plots a histogram of a one dimensional list
"""
import argparse

parser = argparse.ArgumentParser(description='Plot a histogram')

# Required Parameters
parser.add_argument("files", metavar="files", nargs='+', help="files to plot")

# Optional arguments
parser.add_argument("-s", type=str, help="Name of the simulator", default="")
parser.add_argument("-t", type=str, help="Title of the plot", default="")
parser.add_argument("-b", help="Number of bins", default='auto')
parser.add_argument("-n", help="Normed histogram",
                    default=False, action="store_true")

args = parser.parse_args()

import numpy as np
import matplotlib.pyplot as plt
import os

from dim_labels import *


def histogram_plot(data, xlabel, title="", bins='auto', normed=False):
    fig = plt.figure()
    if bins != "auto":
        plt.hist(data, bins=int(bins), density=normed, color='black',
             histtype="bar", rwidth=0.95)
    else:
        plt.hist(data, density=normed, color='black',
             histtype="bar", rwidth=0.95)
    plt.xlabel(xlabel)
    if normed:
        plt.ylabel("Probability")
    else:
        plt.ylabel("Frequency")
    if not title == "":
        plt.title(title)
    return fig

if not os.path.exists("images"):
    os.mkdir("images")

for target_file in args.files:
    #import data
    results = np.recfromtxt(target_file, delimiter=',', loose=True)
    xlabel = DIM_LABELS[target_file.split(".csv")[0].split("_")[-1]]
    if args.t == "":
        title = target_file.split("/")[-1].split("_")[0]
    else:
        title = args.t
    if args.s != "":
        title = title + " for " + SIMULATOR_LABELS[args.s]

    fig = histogram_plot(results, xlabel, title, bins=args.b, normed=args.n)
    fig.savefig(target_file.split(".csv")[0] + ".pdf", format='pdf',
                bbox_inches='tight')
