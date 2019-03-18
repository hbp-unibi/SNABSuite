# Python script that just calls the executable, compatibility hack
# for benchmark service
import sys
arguments = sys.argv[1:]
args_list = ["./benchmark"]
args_list.extend(arguments)

import subprocess
subprocess.check_call(args_list)
