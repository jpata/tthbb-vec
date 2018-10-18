# nanoflow analysis tool

A very simple example on how to access data from NanoAOD is in `src/simple_loop.cc`.
You can compile and run it using

~~~
make simple_loop
./simple_loop input.root
~~~

In this simple example, we need to predefine the event structure in case we want to access it:

~~~
    TTreeReaderValue<unsigned int> nJet(reader, "nJet");
    TTreeReaderArray<float> Jet_pt(reader, "Jet_pt");
    TTreeReaderArray<float> Jet_eta(reader, "Jet_eta");
    TTreeReaderArray<float> Jet_phi(reader, "Jet_phi");
    TTreeReaderArray<float> Jet_mass(reader, "Jet_mass");
~~~

For complex events, it can get tedious to maintain this structure by hand. On the other hand, in PyROOT, you can access the branches simply using `tree.branch_name`, but this results in slow analysis code, as all datatypes are dynamic.

The nanoflow analysis tool is an example of how to access NanoAOD data from C++ in a lazy way without having to predefine the full event structure, but still retain some flexibility of specifying the analysis flow in python.


## Requirements

You need at least GCC 6.2 and ROOT 6.14. Older versions of ROOT (6.10) have some bugs in the TTreePlayer which result in segfaults.
Use the `setup.sh` script to set up the environment on lxplus or other LCG computers.


## Quickstart

~~~
make
./looper data/input_xrootd.json out.json
root -l out.root
~~~


## Overview

The nanoflow analysis code is broken up into the Event, Analyzers and the looper. In the Event we specify the C++ structure of the event. Analyzers process and transform the event and optionally produce some output in the form of histograms, trees etc. The code compiles into a single executable, the `looper`. The `looper` runs all the predefined analyzers and produces an output ROOT file, as well as a report file `out.json` with the job runtime, time required per analyzer etc. All the input arguments to the looper is configured by a json file: `./looper input.json out.json`. In the `input.json` file, we mainly configure the input ROOT files and the output ROOT file, see `data/input_xrootd.json`.

The input datasets are defined in the yaml file in `data/analysis.yaml`. From this, we can generate the input json using

~~~
./python/analysis.py --cache_das
./python/analysis.py --create_jobfiles
./python/analysis.py --run_jobs
~~~

In order to add a new Analyzer, see the examples in `interface/myanalyzers.h`. To change which analyzers are run, simply add them in `src/looper.cc`.