# Pandora

This repo holds the Silent Stores PoCs for [Pandora](https://homes.cs.washington.edu/~dkohlbre/papers/pandora_isca2021.pdf).
Along with the various workloads, our Silent Stores implementation on Gem5 is included under `simulator/`.
The implementation is based on the [Gem5 simulator](https://www.gem5.org/documentation/general_docs/building).
Based on this implementation of silent stores, we presented an attack on Bitslice AES.
The code for this PoC can be found under workloads/bitslice_aes/.
The `workloads` directory includes various other programs used for testing -- including a covert channel based on Silent Stores.

## Paper
Opening Pandora's Box: A Systematic Study of New Ways Microarchitecture Can Leak Private Data.
Jose Rodrigo Sanchez Vicarte, Pradyumna Shome, Nandeeka Nayak, Caroline Trippel, Adam Morrison, David Kohlbrenner, and Christopher W. Fletcher.
Proceedings of the 48th International Symposium on Computer Architecture (ISCA 2021), June 2021.

### Citation

```
@inproceedings{pandora_isca_2021,
  title={Opening Pandora’s Box: A Systematic Study of New Ways Microarchitecture Can Leak Private Data},
  author={Vicarte, Jose Rodrigo Sanchez and Shome, Pradyumna and Nayak, Nandeeka and Trippel, Caroline and Morrison, Adam and Kohlbrenner, David and Fletcher, Christopher W},
  booktitle={2021 ACM/IEEE 48th Annual International Symposium on Computer Architecture (ISCA)},
  pages={347--360},
  year={2021},
  organization={IEEE}
}
```

## How to Use
We have provided various build, run, and processing scripts.
Please note that any modifications to the workloads will likely break the processing scripts.

### Building Simulator/
We have provided (a very simple) build.sh to build the gem5 simulator.
Please note that it will not install dependencies.
All Gem5 dependencies should be setup beforehand, as described in the official build documentation TODO link.

### Available Workloads/
We've made our various testing workloads, and a covert channel implementation, available.
Only the bitslice_aes workload used in our evaluation section is used by run.sh.
Building a workload (using make) creates three versions:
 - debug: Contains additional debug prints and verification of functionality. Runs natively or on gem5 (albeit much slower because of the prints!).
 - native: Contains no additional debug prints or verifications. Runs natively or under gem5.
 - gem5: Contains no additional debug prints or verifications. Does not run natively, because it includes various gem5 magic instructions to properly handle statistics. *This version should be run through gem5 to ensure statistics processing happens correctly*.

### Interpreting Results/
Results can be interpreted using standard Gem5 tactics (we recommend [Konata](https://github.com/shioyadan/Konata)).
Some sample scripts to pull out the target instructions from our workloads are included.
