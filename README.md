# What is nanoflow?

TL;DR: nanoflow is a single-header C++ library to write event loops over NanoAOD - compile your own exe or call from python.

The final stage of LHC collider data analysis is usually in the form of ROOT files containing events formed of a variable number of simple data types (floats, ints). In CMS, this is the NanoAOD step and the structure of the data looks something like this:

~~~
run       : run/I
luminosityBlock : luminosityBlock/i
event     : event/l
nMuon     : nMuon/i
Muon_pt   : Muon_pt[nMuon]/F
Muon_eta  : Muon_eta[nMuon]/F
Muon_phi  : Muon_phi[nMuon]/F
Muon_mass : Muon_mass[nMuon]/F
~~~

Usually, we analyze the data in an explicit event loop. 

## Simple event loop

A very simple example on how to access data from NanoAOD is given in `src/simple_loop.cc`.
You can compile and run it using

~~~
make simple_loop
./bin/simple_loop input.root
~~~

In this simple example, we need to predefine the event structure by hand in case we want to access it:

~~~
    TTreeReaderValue<unsigned int> nJet(reader, "nJet");
    TTreeReaderArray<float> Jet_pt(reader, "Jet_pt");
    TTreeReaderArray<float> Jet_eta(reader, "Jet_eta");
    TTreeReaderArray<float> Jet_phi(reader, "Jet_phi");
    TTreeReaderArray<float> Jet_mass(reader, "Jet_mass");
~~~

For complex events, it gets tedious to maintain this. On the other hand, in Python, you can access the branches simply using `tree.branch_name`, but this results in slow analysis code, as Python needs to carefully check the datatype of any variable before using it.

The nanoflow analysis tool is an example of how to access NanoAOD data from C++ in a lazy way without having to predefine the full event structure, but still retain some flexibility of specifying the analysis flow in python.

## Requirements

You need at least GCC 6.2 and ROOT 6.14. Older versions of ROOT (6.10) have some bugs in the TTreePlayer which result in segfaults. Use the `setup.sh` script to set up the environment on lxplus or other LCG computers.

## Quickstart

~~~
git clone https://gitlab.cern.ch/jpata/nanoflow.git
cd nanoflow
source setup.sh
make
./bin/nf data/input_xrootd.json out.json
root -l out.root
~~~

## Overview

The nanoflow code fits into a single header: `interface/nanoflow.h`. We define our own analysis in `interface/demoanalysis.h` and compile into an executable in `src/nf.cc`.

The `src/nf.cc` file lists the main parts of a nanoflow analysis:
~~~

  //Load a configuration object from a json file
  unique_ptr<Configuration> conf = make_unique<Configuration>(argv[1]);

  //configure the ROOT output file
  unique_ptr<Output> output = make_unique<Output>(conf->output_filename);

  //define the analysis as a sequence of Analyzers
  vector<Analyzer*> analyzers = {
      new MuonEventAnalyzer(*output), //Does something with muons
      new MyTreeAnalyzer(*output) //Writes an output tree
  };

  //Call the event loop, specifying that our event data type is defined in MyAnalysisEvent.
  auto report = looper_main<MyAnalysisEvent, Configuration>(...input_file, configuration...);
~~~

The `MuonAnalyzer` constructs the `Muon` objects from the underlying ROOT TTree and the `MyTreeAnalyzer` specifies what to save to an output TTree. The data structure `MyAnalysisEvent` is defined in `interface/demoanalysis.h` and looks something like this

~~~
class MyAnalysisEvent : public NanoEvent {
 public:
  const Configuration& config;

  // We need to predefine the event content here

  // Physics objects
  vector<Muon> muons;

  // Simple variables
  int nMuon;
  ...
}
~~~

That's it! To get started, either clone this repository and modify `interface/demoanalysis.h` or just download the files `interface/nanoflow.h` and `interface/json.hpp`.

# Analyzing multiple datasets

Usually, your analysis consists of more than one dataset, each representing a different process. Each dataset can be made up of multiple files. We provide a lightweight python library `nanoflow.py` which allows all the datasets to be defined in a single static file `data/analysis.yaml`, and to run the nanoflow code across those files. We can run the nanoflow analysis on these files using 

~~~
./python/analysis.py -a data/analysis.yaml --create_jobfiles --run_jobs
~~~