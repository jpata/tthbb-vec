//In this file, we declare our own custom analyzers, based on the interface defined in nanoflow.h
#ifndef MYANALYZERS_H
#define MYANALYZERS_H

#include <TLorentzVector.h>

#include "nanoflow.h"

TLorentzVector make_lv(float pt, float eta, float phi, float mass);

//We can specialize the LazyObject for specific physics objects
//by wrapping the most commonly used branches.
//Jet type based on the on-demand reading of quantities from the underlying TTree

class Jet : public LazyObject {
public:
    const float _pt, _eta, _phi, _mass;
    Jet(const NanoEvent& _event, unsigned int _index);
    ~Jet() {}

    inline float pt() const {
        return _pt;
    }

    inline float eta() const {
        return _eta;
    }

    inline float phi() const {
        return _phi;
    }

    inline float mass() const  {
        return _mass;
    }

};

class Muon : public LazyObject {
public:
    const float _pt, _eta, _phi, _mass;
    Muon(const NanoEvent& _event, unsigned int _index);
    ~Muon() {}

    inline float pt() const {
        return _pt;
    }

    inline float eta() const {
        return _eta;
    }

    inline float phi() const {
        return _phi;
    }

    inline float mass() const  {
        return _mass;
    }
};

class Electron : public LazyObject {
public:

    const float _pt, _eta, _phi, _mass;

    Electron(const NanoEvent& _event, unsigned int _index);
    ~Electron() {}

    inline float pt() const {
        return _pt;
    }

    inline float eta() const {
        return _eta;
    }

    inline float phi() const {
        return _phi;
    }

    inline float mass() const  {
        return _mass;
    }
};

//This data structure contains the configuration of the event loop.
//Currently, this is only the input and output files.
//We can load the configuration from a json file
class Configuration {
public:
    vector<string> input_files;
    string output_filename;
    
    Configuration(const std::string& json_file);
};

//This is our basic Event representation
//We always construct vectors of basic physics objects such as jets and leptons
//On the other hand, since this is done for every event, we want to keep it as
//minimal as possible.
class Event : public NanoEvent {
public:

    const Configuration& config;

    //We need to predefine the event content here

    //Physics objects
    vector<Jet> jets;
    vector<Muon> muons;
    vector<Electron> electrons;

    //Simple variables
    double lep2_highest_inv_mass;

    Event(TTreeReader& _reader, const Configuration& _config);

    //This is very important to make sure that we always start with a clean
    //event and we don't keep any information from previous events
    void clear_event();

    //In this function we create our event representation
    //In order to have a fast runtime, we need to do the
    //absolute minimum here.
    void analyze();

};

class MuonEventAnalyzer : public Analyzer {
public:
    Output& output;

    MuonEventAnalyzer(Output& _output);
    void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};

class ElectronEventAnalyzer : public Analyzer {
public:
    Output& output;

    ElectronEventAnalyzer(Output& _output);
    void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};


class JetEventAnalyzer : public Analyzer {
public:
    Output& output;

    JetEventAnalyzer(Output& _output);
    void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};

class GenJetEventAnalyzer : public Analyzer {
public:
    Output& output;

    GenJetEventAnalyzer(Output& _output);
    virtual void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};


//This example Analyzer computes the sum(pt) of all the jets, muons and electrons in the event
//and stores it in a histogram. The actual implementation code is located in myanalyzers.cc,
//so we can compile it separately.
class SumPtAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    shared_ptr<TH1D> h_sumpt;
    
    SumPtAnalyzer(Output& _output);
    virtual void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};


//This example Analyzer simply saves the number of primary vertices in an output histogram
class EventVarsAnalyzer : public Analyzer {
public:
    Output& output;

    //Convenient short-hand access to the PV histogram
    shared_ptr<TH1D> h_nPVs;
    EventVarsAnalyzer(Output& _output);
    virtual void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};

class JetJetPair {
public:
    const Jet* j1;
    const Jet* j2;
    const double dr;
    JetJetPair(const Jet& _j1, const Jet& _j2, const double _dr);
};

//Here we compute as an example the deltaR between all jet pairs
class JetDeltaRAnalyzer : public Analyzer {
public:
    Output& output;

    shared_ptr<TH1D> h_deltaR;

    JetDeltaRAnalyzer(Output& _output);
    virtual void analyze(NanoEvent& _event);
    virtual const string getName() const;
};


class LeptonPairAnalyzer : public Analyzer {
public:
    Output& output;

    LeptonPairAnalyzer(Output& _output) : output(_output) {};

    virtual void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};

class MyTreeAnalyzer : public TreeAnalyzer {
public:
    float lep2_highest_inv_mass;
    
    MyTreeAnalyzer(Output& _output);
    virtual void analyze(NanoEvent& _event) override;
    virtual const string getName() const override;
};

//This is the main event loop
//Given a TTreeReader reader, we process all the specified analyzers and store the
//output in the Output data structure.
//You shouldn't have to add anything to the event loop if you want to compute a new
//quantity - rather, you can add a new Analyzer
FileReport looper_main(
    const Configuration& config,
    const string& filename,
    TTreeReader& reader,
    Output& output,
    const vector<Analyzer*>& analyzers,
    long long max_events
    );


#endif
