#!/usr/bin/env python
# -*- coding: utf-8-*-


"""
Plots data from one dimensional sweeps
"""

import argparse
import os
import json

parser = argparse.ArgumentParser(description='Runn all benchmarks')
parser.add_argument("backend", metavar="backend", nargs=1, help="Target backend to run on")

# Optional arguments
parser.add_argument("-n", help="Bench index",type=int, default=0)

args = parser.parse_args()

benchmarks = ["OutputFrequencySingleNeuron",
              "OutputFrequencySingleNeuron2",
              "OutputFrequencyMultipleNeurons",
              "RefractoryPeriod",
              "MaxInputOneToOne",
              "MaxInputAllToAll",
              "MaxInputFixedOutConnector",
              "MaxInputFixedInConnector",
              "SingleMaxFreqToGroup",
              "GroupMaxFreqToGroup",
              "GroupMaxFreqToGroupAllToAll",
              "GroupMaxFreqToGroupProb",
              "SetupTimeOneToOne",
              "SetupTimeAllToAll",
              "SetupTimeRandom",
              "SimpleWTA",
              "LateralInhibWTA",
              "MirrorInhibWTA",
              "WeightDependentActivation",
              "RateBasedWeightDependentActivation",
              "ReluSimilarity",
              "MnistSpikey",
              "MnistNAS63",
              "MnistNAS129",
              "MnistNAStop",
              "MnistDiehl",
              "MnistITLLastLayer",
              "MnistITL",
              "MnistSpikeyTTFS",
              "MnistDiehlTTFS",
              "MnistITLTTFS",
              "BiNAM",
              "BiNAM_pop",
              "BiNAM_burst",
              "BiNAM_pop_burst",
              "SpikingSudoku",
              "SpikingSudokuSinglePop",
              "SpikingSudokuMirrorInhib",
              "SpikingSlam",
              "FunctionApproximation",
              ]
print(args.n)

def parse_result():
    with open(args.backend[0] + "_" + str(args.n) + ".json") as f:
        data = json.load(f)
    return data

result = []

for i in benchmarks:
    os.system("./benchmark " + args.backend[0] + " " + i + " " + str(args.n))
    res = parse_result()
    if res == None:
        continue
    if(res['model'] == i):
        result.append(res)


with open(args.backend[0] + "_" + str(args.n) + "_all.json", 'w') as f:
  json.dump(result, f, indent = 4)


