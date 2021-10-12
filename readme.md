This repo holds the Silent Stores PoCs for [Pandora](https://homes.cs.washington.edu/~dkohlbre/papers/pandora_isca2021.pdf).
Along with the various workloads, our Silent Stores implementation on Gem5 is included under simulator/.
The implementation is based on the [Gem5 simulator](https://www.gem5.org/documentation/general_docs/building).
Based on this implementation of silent stores, we presented an attack on Bitslice AES.
The code for this PoC can be found under workloads/bitslice_aes/.
Also in workloads are various other programs used for testing -- including a covert channel based on Silent Stores.

# How to Use
We have provided various build, run, and processing scripts.
Please note that any modifications to the workloads will likely break the processing scripts.

## Building Simulator/
We have provided (a very simple) build.sh to build the gem5 simulator.
Please note that it will not install dependencies.
All Gem5 dependencies should be setup beforehand, as described in the official build documentation TODO link.

## Available Workloads/
We've made our various testing workloads, and a covert channel implementation, available.
Only the bitslice_aes workload used in our evaluation section is used by run.sh.
Building a workload (using make) creates three versions:
 - debug: Contains additional debug prints and verification of functionality. Runs natively or on gem5 (albeit much slower because of the prints!).
 - native: Contains no additional debug prints or verifications. Runs natively or under gem5.
 - gem5: Contains no additional debug prints or verifications. Does not run natively, because it includes various gem5 magic instructions to properly handle statistics. *This version should be run through gem5 to ensure statistics processing happens correctly*.

## Interpreting Results/
