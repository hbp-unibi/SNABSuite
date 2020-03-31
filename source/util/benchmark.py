# Python script that just calls the executable, compatibility hack
# for benchmark service
import sys
arguments = sys.argv[1:]

simulators = ["nest", "spinnaker", "nmpm1", "spikey"]  # SNABSuite simulators
web_simulators = ["NEST", "SpiNNaker", "BrainScaleS",
                  "Spikey"]  # Web frontend simulators
web_simulators_low = ["nest", "spinnaker", "brainscales",
                  "spikey"]  # Web frontend simulators

index = web_simulators_low.index(arguments[0].lower())
arguments[0] = simulators[index]

args_list = ["./benchmark"]
args_list.extend(arguments)

import subprocess
subprocess.check_call(args_list)

import os
for root, dirs, files in os.walk(os.path.abspath("./")):
    for file in files:
        if not file.endswith('.json'):
            os.remove(os.path.join(root, file))
