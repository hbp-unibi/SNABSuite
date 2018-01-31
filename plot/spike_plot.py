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
Plots spike time data 
"""

import argparse

parser = argparse.ArgumentParser(description='Plot spike times')

# Required Parameters
parser.add_argument("files", metavar="files", nargs='+', help="files to plot")

# Optional arguments
parser.add_argument("-s", type=str, help="Name of the simulator", default="")
parser.add_argument("-p", help="Save as png instead of pdf",
                    default=False, action="store_true")

args = parser.parse_args()

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import os

from dim_labels import *

# Adapted from https://gist.github.com/kylerbrown/5530238
def raster_plot(spikes, simulator):
    fig = plt.figure()
    ax = plt.gca()
    for ith, trial in enumerate(spikes):
        plt.vlines(trial, ith + .5, ith + 1.5,  color="#000000", lw=0.5)
    ax.set_ylim(.5, len(spikes) + .5)
    if simulator == "":
        ax.set_title("Spike Times")
    else:
        ax.set_title("Spike Times for " + SIMULATOR_LABELS[simulator])
    ax.set_xlabel(DIM_LABELS["time"])
    ax.set_ylabel(DIM_LABELS["neuron id"])
    if len(spikes)>1:
        ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    else:
        ax.set_yticks([1])
    return fig

for target_file in args.files:
    #import data
    data = []
    with open(target_file) as file:
        for line in file:
            data2 = []
            for entry in line.split(","):
                try:
                    data2.append(float(entry))
                except:
                    continue
            data.append(data2)

    fig = raster_plot(data, args.s)

    if args.p:
        fig.savefig(target_file.split(".csv")[0] + ".png", format='png', 
                    dpi=600, bbox_inches='tight')
    else:
        fig.savefig(target_file.split(".csv")[0] + ".pdf", format='pdf',
                    bbox_inches='tight')
