# nanoflow analysis tool

The nanoflow analysis tool is an example of how to access data from NanoAOD in a lazy
way without having to predefine the full event structure.


## Requirements

You need at least GCC 6.2 and ROOT 6.14. Older versions of ROOT (6.10) have some bugs in the TTreePlayer
which result in segfaults.


## Quickstart

~~~
make looper
./looper data/input_xrootd.json out.json
root -l out.root
~~~


## Overview

The nanoflow analysis code is broken up into Analyzers, each of which can process or modify the event in some
way and produce some output in the form of histograms, trees etc. The code compiles into a single
executable, the `looper`. The `looper` runs all the predefined analyzers and produces an output ROOT file,
as well as a report file `out.json` with the job runtime, time required per analyzer etc. All the input arguments to the looper
is configured by a json file: `./looper input.json out.json`. In the `input.json` file, we mainly configure
the input ROOT files and the output ROOT file, see `data/input_xrootd.json`.

The input datasets are defined in the yaml file in `data/analysis.yaml`. From this, we can generate the input
json using `python python/analysis.py --das_cache --create_jobfiles`.

In order to add a new Analyzer, see the examples in `interface/myanalyzers.h` and `interface/myanalyzers.cc`.
To change which analyzers are run, you currently need to edit `src/looper.cc`.
If you need to modify the Event data structure, see `interface/nanoflow.h` and `src/nanoflow.cc`.