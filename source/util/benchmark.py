# Python script that just calls the executable, compatibility hack
# for benchmark service
import sys
arguments = sys.argv[1:]

simulators = ["nest", "spinnaker", "nmpm1", "spikey"]  # SNABSuite simulators
web_simulators = ["NEST", "SpiNNaker", "BrainScaleS",
                  "Spikey"]  # Web frontend simulators

index = web_simulators.index(arguments[0])
arguments[0] = simulators[index]

args_list = ["./benchmark"]
args_list.extend(arguments)

import subprocess
subprocess.check_call(args_list)
