# SNABSuite
![Logo](https://raw.github.com/hbp-unibi/SNABSuite/master/snapsuit.png)

## Spiking Neural Architecture Benchmark Suite

This project provides a set of basic benchmarks (SNABs) for neuromorphic simulators like NEST, the Spikey chip or the SpiNNaker system. Benchmark networks are described in the platform-agnostic Cypress framework providing a black-box approach to comparing different simulation backends. To account for hardware differences, every SNAB relies on a config file. Results are presented in a JSON-like structure.

[![Build Status](https://travis-ci.org/hbp-unibi/SNABSuite.svg?branch=master)](https://travis-ci.org/hbp-unibi/SNABSuite) [![Documentation](https://img.shields.io/badge/docs-doxygen-blue.svg)](https://hbp-unibi.github.io/SNABSuite/index.html)

## Installation and Usage

Compiling the project requires an installation of the cypress framework and thus has the same requirements. To set-up the benchmarks suite, run
```bash
git clone https://github.com/hbp-sanncs/SNABSuite
mkdir SNABSuite/build && cd SNABSuite/build
cmake ..
make && make test
```

Afterwards you can execute all benchmarks consecutively by
```bash
./benchmark <platform> [snab] [bench_index] [NMPI]
```
Here, the `<platform>` can be substituted by all platforms currently supported by Cypress. This list contains SpiNNaker, Spikey, BrainScaleS (and its executable system specification) and nest/pynn.nest as well as genn. Note that the respective backend has to be installed on your system. 
Optional arguments:
 * [snab] can be a name of a specific benchmark, if you are only interested in the specific outcome of a single benchmark
 * [bench_index] This interesting for scalable benchmarks. Defaults to 0, this integer refers to the overall network size. 0 represents single core networks, 1 single chip, 2 small board and 3 to large scale networks. If a SNAB is not scaleble, or larger networks are not supported, the evaluation will be skipped
 * [NMPI] write NMPI for remote execution on HBP hosts. Requires an ebrains account
 

## Overall Architecture/Hints for adding a new SNAB

The benchmark/SNAB base class can be found in `source/common/snab_base.hpp`. New SNABs have to implement the virtual functions given. The constructor of your SNAB should initialize the base class correctly to make evaluation possible. Details can be found in the documentation. By activating the constructor of the base class, a config file is read in, which should be a JSON file in the config-directory with the same name as the SNAB. This config contains all platform specific parameters which are set in the individual SNABs. 
Finally, all SNABs are registered in the `source/common/snab_registry.hpp` file, to make a consecutive execution of all SNABs possible.
To help implementing new SNABs, there is a list of common tools and utilities found in their respective directories. For example there is a NeuronParameters class which helps handling the different neuron implementations of the cypress framework.

### Config files

Here all the platform specific things are gathered. A few examples how a config file can look like can be found in the config directory. The platform specific entries are not limited to neuron parameters. Everything needed for "building" the SNAB in Cypress can be put into the file, like populations sizes or connectivity probabilities. This completely depends on the SNAB implementation. 
However, there are some general things which are shortcutted by the base implementation.
You can mark platforms as invalid:
```javascript
"nest" : {
    "invalid" : true
}
```
or define default parameters:
```javascript
"default" : {
    ...
}
```
which are taken if there is no platform specific entry. If there is no default defined and no configuration found, the system takes the configuration of one of the other platforms as a last try.

### Parameter Sweeps

The suite comes with an integrated parameter sweep framework. 
```bash 
./sweep <SIMULATOR> <SWEEP_CONFIG> <bench_index> [threads] [NMPI]
```
For the meaning of individual parameters please look at the documentation of `./benchmark`. 
`[threads]` will have an impact on non-python simulators like genn and nest (not pynn.nest!).

The sweep config refers to a file defining the parameter sweep. Examples of these can be found in "sweeps" folder. In general it must look like this
```javascript
{
    "snab_name" : "test_name"
    "nest" : {
        "test_param" : [startvalue, endvalue, #steps],
        "top_key" : {
            "sub_key": [startvalue1, endvalue1, #steps1]
        } 
    }
}
```
Here, the config file for the snab/benchmark itself must contain the values "test_param" and "top_key"/"sub_key". Internally, these values are overwritten by generated values in the given intervals [startvalue*,endvalue*]. 

The result of such a parameter sweep will be stored as a csv. For plotting results, the `plot` folder contains the scripts `1dim_plot.py` and `2dim_plot.py` for 1/2 dimensional sweeps. Labels for dimenstions should usually be included in `plot/dim_labels.py`. 

Finally, the parameter sweep has a backup functionality included. If the sweep breaks down for whatever reason, there will be a backup `simulator_bak.json`. Restarting the same sweep will search for such a backup file and continue, while automatically retrying those runs with invalid results.

### Debugging SNABs
SNABSuite has an inbuilt compilation switch, that triggers extensive output of data to storage at network runtime. For many SNABs this means, that spike data is recorded and plotted.
This is tiggered by calling 
```bash
cmake .. -DSNAB_DEBUG=true
make
```
and re-running the benchmark. Output data can be found in a subfolder called `debug`.
Alternativly, this can also be configured with a graphical tool calling `ccmake ..` from your build folder. Here, you can see various other configuration switches, that you usully not need to change.
Before running parameter sweeps, we recommend to turn it off again (setting SNAB_DEBUG to false and recompile), as every run triggers writing of data and a new instance of a Python interpreter plotting the data.


# Related Publications
* A. Stöckel et al., “Binary Associative Memories as a Benchmark for Spiking Neuromorphic Hardware,” Front. Comput. Neurosci., vol. 11, no. August, p. 71, Aug. 2017, doi: 10.3389/fncom.2017.00071.
* C. Ostrau et al., “Benchmarking Deep Spiking Neural Networks on Neuromorphic Hardware,” Apr. 2020, arXiv:2004.01656.
* C. Ostrau et al., “Benchmarking of Neuromorphic Hardware Systems,” in Proceedings of the Neuro-inspired Computational Elements Workshop, 2020, pp. 1–4, doi: 10.1145/3381755.3381772.
* C. Ostrau et al., “Benchmarking and Characterization of event-based Neuromorphic Hardware,” 2019.
* C. Ostrau et al., “Comparing Neuromorphic Systems by Solving Sudoku Problems,” in Conference Proceedings: 2019 International Conference on High Performance Computing & Simulation (HPCS), 2019.

# Acknowledgement
This project has been developed at Bielefeld University. It has received funding from the European Union Seventh Framework Programme (FP7) under grant agreement no 604102 and the EU’s Horizon 2020 research and innovation programme under grantagreements No 720270 and 785907 (Human Brian Project, HBP). It has been further supported by the Cluster of Excellence Cognitive Interaction Technology “CITEC” (EXC 277) at Bielefeld University, which is funded by the German Research Foundation (DFG).  
