# What is nanoflow?

TL;DR: nanoflow is a single-header C++ library to write event loops over NanoAOD - compile your own exe or call from python.

The final stage of LHC collider data analysis is usually in the form of ROOT files containing events formed of a variable number of simple data types (floats, ints). In CMS, this is the NanoAOD step and the structure of the data looks something like this:

~~~
run/I
luminosityBlock/i
event/l
nMuon/i
Muon_pt[nMuon]/F
Muon_eta[nMuon]/F
Muon_phi[nMuon]/F
Muon_mass[nMuon]/F
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
source setup.sh #or setup-slc6.sh on lxplus or other SLC6 machine
make
./bin/nf data/input_xrootd.json out.json
~~~

This will run the event loop and print out some metrics.
~~~
$ ./bin/nf data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/job_0.json out.json
18:28:12 nanoflow main() started
18:28:13 loading json file data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/job_0.json
Creating output file ./data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/out_0.root
Creating output file ./data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/out_0.root
Creating Analyzers
Creating MuonEventAnalyzer
Opening input file file:///Users/joosep/Documents/caltech/nanoflow/./data/local_cache/skim_ZZTo2e2mu.root
starting loop over 50000 events in TTree 0x7fb308298510
Processed 0/50000 speed=0kHz ETA=infs
Processed 10000/50000 speed=89.0416kHz ETA=0.449228s
Processed 20000/50000 speed=131.152kHz ETA=0.228742s
Processed 30000/50000 speed=155.894kHz ETA=0.128292s
Processed 40000/50000 speed=169.306kHz ETA=0.0590645s
looper_main nevents=50000,cpu_time=0.26,real_time=0.27646,speed=180.858
FileReport eff=0.940462,NanoEvent=0.298338,MuonEventAnalyzer=0.334345,MyTreeAnalyzer=0.367317,
All input files processed, saving output
Writing output to file ./data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/out_0.root:/
18:28:14 nanoflow main() done on json file data/workdir/jobs/ZZTo2e2mu/ZZTo2e2mu/job_0.json
~~~

## Overview

The nanoflow code fits into a single header: `interface/nanoflow.h`. We define our own analysis in `interface/demoanalysis.h` and compile into an executable in `src/nf.cc`.

The `src/nf.cc` file lists the main parts of a nanoflow analysis:
~~~

  //Load a configuration object from a json file
  Configuration> conf(argv[1]);

  //configure the ROOT output file
  Output output(conf.output_filename);

  //define the analysis as a sequence of Analyzers
  vector<Analyzer*> analyzers = {
      new MuonEventAnalyzer(*output), //Does something with muons
      new MyTreeAnalyzer(*output) //Writes an output tree
  };

  //Call the event loop, specifying that our event data type is defined in MyAnalysisEvent.
  auto report = looper_main<MyAnalysisEvent, Configuration>(conf, reader, output, analyzers);

  //Print the overall CPU efficiency and fraction of time spent per analyzer
  report.print(cout);
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

That's it! To get started, either clone this repository and modify `interface/demoanalysis.h` or just download the files `interface/nanoflow.h` and `interface/json.hpp` to use in your own project. 

# Analyzing multiple datasets

Usually, your analysis consists of more than one dataset, each representing a different process. Each dataset can be made up of multiple files. We provide a lightweight python library `nanoflow.py` which allows all the datasets to be defined in a single static file `data/analysis.yaml`, and to run the nanoflow code across those files. We can run the nanoflow analysis on these files using 

~~~
./python/analysis.py -a data/analysis.yaml --create_jobfiles --run_jobs
~~~
