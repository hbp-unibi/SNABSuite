# SNABSuite
## Spiking Neural Architecture Benchmark Suite

This project provides a set of basic benchmarks (SNABs) for neuromorphic simulators like NEST, the Spikey chip or the SpiNNaker system. Benchmark networks are described in the platform-agnostic Cypress framework providing a black-box approach to comparing different simulation backends. To account for hardware differences, every SNAB relies on a config file. Results are presented in a JSON-like structure.

[![Build Status](https://travis-ci.org/hbp-unibi/SNABSuite.svg?branch=master)](https://travis-ci.org/hbp-unibi/SNABSuite)

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
./benchmark <platform>
```
Here, the <platform> can be substituted by all platforms currently supported by Cypress. This list contains SpiNNaker, Spikey, BrainScaleS (and its executable system specification) and nest/pynn.nest. Note that the respective backend has to be installed on your system. 

## Overall Architecture/Hints for adding a new SNAB

The benchmark/SNAB base class can be found in `source/common/snab_base.hpp`. New SNABs have to implement the virtual functions given. The constructor of your SNAB should initialize the base class correctly to make evaluation possible. Details can be found in the documentation. By activating the constructor of the base class, a config file is read in, which should be a JSON file in the config-directory with the same name as the SNAB. This config icontains all platform specific parameters which are set in the individual SNABs. 
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
