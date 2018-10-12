//This file contains helpers for doing Analyzer-based event processing on NanoAOD with an event loop
#ifndef NANOFLOW_H
#define NANOFLOW_H

#include <chrono>
#include <iomanip>

#include <TFile.h>
#include <TH1D.h>
#include <TStopwatch.h>

#include "json.hpp"
#include "nanoevent.h"

using namespace std;
using nlohmann::json;

//This is our basic Event representation
//We always construct vectors of basic physics objects such as jets and leptons
//On the other hand, since this is done for every event, we want to keep it as
//minimal as possible.
class Event : public NanoEvent {
public:

    //We need to predefine the event content here, such that 
    vector<Jet> jets;
    vector<Muon> muons;
    vector<Electron> electrons;

    double highest_inv_mass;
    Event(TTreeReader& _reader) : NanoEvent(_reader) {}

    //This is very important to make sure that we always start with a clean
    //event and we don't keep any information from previous events
    void clear_event();

    //In this function we create our event representation
    void analyze();

};

//This is a simple template of an Analyzer
//An Analyzer receives a read-only event and processes it in some simple way
class Analyzer {
public:
    virtual void analyze(Event& event) = 0;
    virtual const string getName() const = 0;
};


//A class that creates the output file and contains all the other
//output objects: histograms, TTrees etc
class Output {
public:
    unique_ptr<TFile> outfile;

    //A map of "histo_nickname" -> TH1D
    //However, for reasons of speed, we use a compile-time hash of the string
    //therefore, we only ever refer to the histogram by its hash, which is a number
    unordered_map<unsigned int, shared_ptr<TH1D>> histograms_1d;
    unordered_map<unsigned int, shared_ptr<TTree>> trees;

    //Creates the output TFile
    Output(const string& outfn);

    //makes sure the TFile is properly written and closed
    void close();
};


//This is an example of how to produce TTree outputs
class TreeAnalyzer : public Analyzer {
public:
    Output& output;

    shared_ptr<TTree> out_tree;
    TreeAnalyzer(Output& _output);
   
    unsigned int br_run;
    unsigned int br_luminosityBlock;
    unsigned long br_event;
 
    virtual void analyze(Event& event);
    virtual const string getName() const;
};

//This data structure contains the configuration of the event loop.
//Currently, this is only the input and output files.
//We can load the configuration from a json file
class Configuration {
public:
    vector<string> input_files;
    string output_filename;

    Configuration(const string& json_file);
};


class FileReport {
public:
    //Keeps track of the total duration (in nanoseconds) spent on constructing
    //the event representation from the ROOT file 
    unsigned long long event_duration;

    //total number of events processed
    unsigned long long num_events_processed;

    //Event loop timing information
    double cpu_time;
    double real_time;
    double speed;

    //input filename
    string filename;

    //Keeps track of the total duration (in nanoseconds) spent on each analyzer
    vector<unsigned long long> analyzer_durations;
    
    vector<string> analyzer_names;

    FileReport(const string& filename, const vector<Analyzer*>& analyzers);

    void print(std::ostream& stream);
};

void to_json(json& j, const FileReport& p);

//This is the main event loop
//Given a TTreeReader reader, we process all the specified analyzers and store the
//output in the Output data structure.
//You shouldn't have to add anything to the event loop if you want to compute a new
//quantity - rather, you can add a new Analyzer
FileReport looper_main(
    const string& filename,
    TTreeReader& reader,
    Output& output,
    const vector<Analyzer*>& analyzers,
    long long max_events = -1);

//
//Utility functions
//

//Prints the system time as HH::mm::ss
string get_time();

#endif
