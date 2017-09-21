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
Plots data from one dimensional sweeps
"""
import argparse

parser = argparse.ArgumentParser(description='Plot one-dimensional graphs')

# Optional arguments
parser.add_argument("-nx", help="normalize x-values",  action="store_true")
parser.add_argument("-ny", help="normalize y-values", action="store_true")
parser.add_argument("--ymin", type=float, help="minimal y-value")
parser.add_argument("--ymax", type=float, help="maximal y-value")
parser.add_argument(
    "-ys", type=int, help="Column of std-deviation of y-values")
parser.add_argument("-x", type=int, help="Column of x-values", default=0)

# Required Parameters
parser.add_argument("-y", type=int, required=True, help="Column of y-values")
parser.add_argument("files", metavar="files", nargs='+', help="files to plot")

args = parser.parse_args()


import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colorbar
import sys
import os

from dim_labels import *


def cm2inch(value):
    return value / 2.54


def plot_measure(ax, xs, ys, ys_std, color, simulator, xlabel, ylabel,
                 ys_ref=None, first=True, ymin=None, ymax=None):

    ax.plot(xs, ys, color=color, lw=1.0, zorder=1, label=simulator)

    if ys_std is not None:
        ax.plot(xs, ys - ys_std * 0.5, lw=0.5,
                linestyle=':', color=color, zorder=0)
        ax.plot(xs, ys + ys_std * 0.5, lw=0.5,
                linestyle=':', color=color, zorder=0)

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    if (ymin is None) and (ymax is None):
        ax.autoscale_view()
    elif ymax is None:
        ax.set_ylim(bottom=ymin)
    elif ymin is None:
        ax.set_ylim(top=ymax)
    else:
        ax.set_ylim(bottom=ymin, top=ymax)


def normalize(data, norm):
    for i in xrange(0, len(data)):
        data[i] = data[i] / norm


def get_max(data):
    idcs = np.isfinite(data)
    return np.max(data[np.isfinite(data)])


def get_min(data):
    idcs = np.isfinite(data)
    return np.min(data[np.isfinite(data)])

fig = plt.figure(figsize=(cm2inch(12), cm2inch(6.0)))
ax = fig.add_subplot(111)

for target_file in args.files:
    results = np.genfromtxt(target_file, delimiter=',', names=True)
    keys = results.dtype.names
    data = np.zeros((results.shape[0], len(keys)))
    for i in xrange(0, len(results)):
        data[i] = np.array(list(results[i]))

    xs = np.array(data[:, args.x])
    ys = np.array(data[:, args.y])
    ys_dev = None
    if args.ys:
        ys_dev = np.array(data[:, args.ys])

    if args.nx:
        normalize(xs, np.abs(get_max(xs)))

    if args.ny:
        normalize(ys, np.abs(get_max(ys)))
        if args.ys:
            normalize(ys_dev, np.abs(get_max(ys)))

    simulator = target_file.split('_')[-1].split('.csv')[0].split('.')[-1]
    plot_measure(ax, xs, ys, ys_dev, color=SIMULATOR_COLORS[simulator],
                 simulator=SIMULATOR_LABELS[simulator], xlabel=DIM_LABELS[keys[args.x]],
                 ylabel=DIM_LABELS[keys[args.y]], ymin=args.ymin, ymax=args.ymax)

if not os.path.exists("images"):
    os.mkdir("images")
ax.legend(loc='lower center', bbox_to_anchor=(0.5, 1.05),
          ncol=4)
if args.files[-1].split('/')[-2]:
    fig.savefig("images/" + args.files[-1].split('/')[-2] + "_" + args.files[-1].split(
        '/')[-1].split('_')[0] + ".pdf", format='pdf', bbox_inches='tight')
else:
    fig.savefig("images/" + args.files[-1].split('/')[-1].split('_')
                [0] + ".pdf", format='pdf', bbox_inches='tight')
