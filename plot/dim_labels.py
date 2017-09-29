#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   SNABSuite -- Spiking Neural Architecture Benchmark Suite
#   Copyright (C) 2017, Christoph Jenzen
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
# Labels for all possible sweep dimensions (wip)
"""

DIM_LABELS = {
    "n_bits": "Memory size $n, m$",
    "n_bits_in": "Input vector length $m$",
    "n_bits_out": "Output vector length $n$",
    "n_ones_in": "Number of ones in the input $c$",
    "n_ones_out": "Number of ones in the input $d$",
    "n_samples": "Number of samples $N$",
    "cm": "Membrane capacitance $C_M$ [nF]",
    "e_rev_E": "Excitatory reversal potential $E_E$ [mV]",
    "e_rev_I": "Inhibitory reversal potential $E_I$ [mV]",
    "v_rest": "Resting potential $E_L$ [mV]",
    "v_reset": "Reset potential $E_{\\mathrm{reset}}$ [mV]",
    "v_thresh": "Threshold potential $E_{\\mathrm{Th}}$ [mV]",
    "g_leak": "Leak conductivity $g_\\mathrm{L}$ [$\\mu\\mathrm{S}$]",
    "tau_m": "Membrane time constant $\\tau _m$ [ms]",
    "tau_syn_E": "Excitatory time constant $\\tau_\\mathrm{e}$ [ms]",
    "tau_syn_I": "Inhibitory time constant $\\tau_\\mathrm{i}$ [ms]",
    "multiplicity": "Neuron population size $s$",
    "weight": "Synapse weight $w$ [$\\mu\\mathrm{S}$]",
    "topology.sigma_w": "Synapse weight noise $\\sigma_w$ [$\\mu \\mathrm{S}$]",
    "burst_size": "Input burst size $s$",
    "time_window": "Time window $T$ [ms]",
    "isi": "Burst inter-spike-interval $\Delta t$ [ms]",
    "sigma_t": "Spike time noise $\sigma_t$ [ms]",
    "sigma_offs": "Spike time offset noise $\sigma_t^{\\mathrm{offs}}$ [ms]",
    "Average_frequency": "Average frequency [1/ms]",
    "ConnectionsPerInput": "\\#Connections per source neuron / \\#Target neurons",
    "Standard_deviation": "Standard deviation",
    "Average_number_of_spikes": "Normalized average spike count",
    "neurons": "Number of Neurons",
    "Average_frequency_of_neurons": "Average frequency [1/ms]",
    "Average_deviation_from_refractory_period": "Average deviation [ms]",
    "time" : "Time in [ms]",
    "neuron id" : "Neuron ID"
}


def get_label(key):
    return DIM_LABELS[key] if key in DIM_LABELS else key


SIMULATOR_LABELS = {
    "ess": "ESS",
    "nmpm1": "NM-PM1",
    "spiNNaker": "SpiNNaker",
    "spinnaker": "SpiNNaker",
    "spinnaker2": "SpiNNaker@0.1ms",
    "spikey": "Spikey",
    "nest": "NEST",
    "pynn": "NEST",
    "pynn.nest": "NEST",
}

# Colors for all simulators
SIMULATOR_COLORS = {
    "ess": '#73d216',
    "nmpm1": "#75507b",
    "spiNNaker": "#f57900",
    "spinnaker": "#f57900",
    "spinnaker2": "#000000",
    "spikey": "#cc0000",
    "nest": "#3465a4",
    "pynn": "#3465a4",
    "pynn.nest": "#3465a4",
}
