/**
 * @mainpage SNABSuite
 * @section Spiking Neural Architecture Benchmark Suite
 * This project provides a set of basic benchmarks (SNABs) for neuromorphic simulators like NEST, the Spikey chip or the SpiNNaker system. Benchmark networks are described in the platform-agnostic Cypress framework providing a black-box approach to comparing different simulation backends. To account for hardware differences, every SNAB relies on a config file. Results are presented in a JSON-like structure.
 * 
 * @section Benchmarks
 * All benchmark implementations inherit from SNAB::SNABBase so that a list of implementet benchmarks can be found at SNAB::SNABBase.
 * 
 * @section Implementation
 * Benchmarks are implemented using the backend agnostic framework [cypress](https://github.com/hbp-unibi/cypress).
 * 
 */
