#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   SNABSuite -- Spiking Neural Architecture Benchmark Suite
#   Copyright (C) 2017 Andreas Stöckel, Christoph Jenzen
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
Plots data for two dimensional sweeps
"""
from __future__ import division

from builtins import range
from past.utils import old_div
import argparse

parser = argparse.ArgumentParser(description='Plot two-dimensional images')

# Optional arguments
parser.add_argument("--zmin", type=float, help="minimal z-value")
parser.add_argument("--zmax", type=float, help="maximal z-value")
parser.add_argument(
    "-nl", type=int, help="Number of levels/ticks in z", default=11)
parser.add_argument("-q", help="qualitative Colormap",  action="store_true")
parser.add_argument("-c", help="draw contour lines",  action="store_true")


# Required Parameters
parser.add_argument("-z", type=int, required=True, help="Column of z-values")
parser.add_argument("files", metavar="Files", nargs='+',  help="files to plot")

args = parser.parse_args()

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colorbar
import sys
import os
from dim_labels import *


def cm2inch(value):
    return value / 2.54


def round_to_divisable(value, divisable):
    if value == 0:
        return 0
    temp = np.abs(value)
    a = 0
    while temp < divisable:
        temp *= 10.0
        a += 1
    if temp % divisable == 0:
        return value
    res = old_div((temp - (temp % divisable) + divisable), (10.0**a))
    if value < 0:
        return -res
    return res


def plot_measure2d(xs, ys, zs, xlabel, ylabel, zlabel="", zmin=None,
                   zmax=None, qualitative=False, contour=True, title=None):
    fig = plt.figure(figsize=(cm2inch(5.5), cm2inch(5.5)))
    
    ax1 = fig.add_axes([0.0, 0.25, 1.0, 0.85])
    if title is not None:
        plt.title(title)
    ax2 = fig.add_axes([0.0, 0.0, 1.0, 0.05])
    

    _, steps_x = np.unique(xs, return_counts=True)
    _, steps_y = np.unique(ys, return_counts=True)
    steps_x = np.max(steps_x)
    steps_y = np.max(steps_y)
    xs = xs.reshape((steps_y, steps_x))
    ys = ys.reshape((steps_y, steps_x))
    zs = zs.reshape((steps_y, steps_x))
    zs = zs.transpose()

    # Auto-scale
    idcs = np.isfinite(zs)
    if np.sum(idcs) == 0:
        return
    if zmin is None:
        zmin = np.min(zs[idcs])
        if 0 < zmin < 1:
            zmin = 0
        else:
            zmin = int(zmin)
    if zmax is None:
        zmax = round_to_divisable(np.max(zs[idcs]), args.nl - 1)
        if zmin > 0:
        	zmax = zmax + zmin
        if 0 < zmax < 1:
            zmax = 1

    # Select the colormap
    if qualitative:
        cmap = plt.cm.rainbow
    else:
        #cmap = plt.cm.Purples
        # if zmin < 0.0:
        cmap = plt.cm.PuOr
    cmap.set_bad('black', 1.)

    extent = (np.min(xs), np.max(xs), np.min(ys), np.max(ys))
    ax1.imshow(zs, aspect='auto', origin='lower', extent=extent, cmap=cmap,
               vmin=zmin, vmax=zmax, interpolation="none")

    levels = np.linspace(zmin, zmax, args.nl)
    zs = zs.transpose()
    if contour:
        CS2 = ax1.contour(xs, ys, zs, levels, linewidths=0.25,
                          colors='k', vmin=zmin, vmax=zmax)
    ax1.grid(color='black', linestyle=':', linewidth=0.25)
    ax1.set_xlabel(xlabel)
    ax1.set_ylabel(ylabel)
    cbar = matplotlib.colorbar.ColorbarBase(ax2, cmap=cmap,
                                            orientation='horizontal', ticks=levels,
                                            norm=matplotlib.colors.Normalize(zmin, zmax))
    cbar.set_label(zlabel)
    return fig


if not os.path.exists("images"):
    os.mkdir("images")

for target_file in args.files:
    simulator = target_file.split('_')[-1].split('.csv')[0]
    experiment = target_file.split('/')[-1].split(simulator)[0]

    #import data
    results = np.genfromtxt(target_file, delimiter=',', names=True)
    keys = results.dtype.names
    data = np.zeros((results.shape[0], len(keys)))
    for i in range(0, len(results)):
        data[i] = np.array(list(results[i]))

    fig = plot_measure2d(data[:, 0], data[:, 1], data[:, args.z],
                         xlabel=get_label(keys[0]), ylabel=get_label(keys[1]),
                         zlabel=get_label(keys[args.z]), zmin=args.zmin,
                         zmax=args.zmax, qualitative=args.q, contour=args.c,
                         title=SIMULATOR_LABELS[simulator])

    if target_file.split('/')[-2]:
        if not os.path.exists("images/" + target_file.split('/')[-2]):
            os.mkdir("images/" + target_file.split('/')[-2])
        fig.savefig("images/" + target_file.split('/')[-2] + "/" +
                    experiment + simulator + ".pdf", format='pdf',
                    bbox_inches='tight')
    else:
        fig.savefig("images/" + experiment + simulator + ".pdf", format='pdf',
                    bbox_inches='tight')
