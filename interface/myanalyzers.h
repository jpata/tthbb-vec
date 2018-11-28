// In this file, we declare our own custom analyzers, based on the interface
// defined in nanoflow.h
#ifndef MYANALYZERS_H
#define MYANALYZERS_H

#include <TLorentzVector.h>

#include "nanoflow.h"

// Helper function to create a TLorentzVector from spherical coordinates
TLorentzVector make_lv(float pt, float eta, float phi, float mass);

//Objects that derive from this class contain the spherical components of
//four-momentum and accessor functions for those.
//These properties cannot be modified after creation for safety.
class FourMomentumSpherical {
 public:
  const double _pt, _eta, _phi, _mass;
  FourMomentumSpherical() : _pt(0.0), _eta(0.0), _phi(0.0), _mass(0.0) {}
  FourMomentumSpherical(double pt, double eta, double phi, double mass)
      : _pt(pt), _eta(eta), _phi(phi), _mass(mass) {}
  inline double pt() const { return _pt; }

  inline double eta() const { return _eta; }

  inline double phi() const { return _phi; }

  inline double mass() const { return _mass; }
};


//
//Physics object datatypes
//


// We can specialize the LazyObject for specific physics objects
// by wrapping the most commonly used branches.
// Jet type based on the on-demand reading of quantities from the underlying
// TTree. Such objects are constructed directly from the underlying
// TTree data by referring to the branches in the constructor.
class Muon : public LazyObject, public FourMomentumSpherical {
 public:
  int matchidx = -1;

  Muon(NanoEvent* _event, unsigned int _index);
  ~Muon() {}
};

// This data structure contains the configuration of the event loop.
// We can load the configuration from a json file
class Configuration {
 public:
  vector<string> input_files;
  string output_filename;
  int max_events;

  Configuration(const std::string& json_file);
};

// This is our basic Event representation
// We always construct vectors of basic physics objects such as jets and leptons
// On the other hand, since this is done for every event, we want to keep it as
// minimal as possible.
class MyAnalysisEvent : public NanoEvent {
 public:
  const Configuration& config;

  // We need to predefine the event content here

  // Physics objects
  vector<Muon> muons;

  // Simple variables
  int nMuon;

  MyAnalysisEvent(TTreeReader& _reader, const Configuration& _config);

  // This is very important to make sure that we always start with a clean
  // event and we don't keep any information from previous events
  void clear_event();

  // In this function we create our event representation
  // In order to have a fast runtime, we need to do the
  // absolute minimum here.
  void analyze();
};

//
//Data analyzers
//

class MuonEventAnalyzer : public Analyzer {
 public:
  Output& output;

  MuonEventAnalyzer(Output& _output);
  void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

//Specification of the output
class MyTreeAnalyzer : public TreeAnalyzer {
 public:
  float lep2_highest_inv_mass;
  int nMuon_match;

  int nMuon;
  array<float, 10> Muon_px;
  array<float, 10> Muon_py;
  array<float, 10> Muon_pz;
  array<float, 10> Muon_energy;
  array<int, 10> Muon_matchidx;

  MyTreeAnalyzer(Output& _output);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
  void clear();

  void fill_muon(MyAnalysisEvent& event, vector<Muon>& src);
};

// This is the main event loop
// Given a TTreeReader reader, we process all the specified analyzers and store
// the  output in the Output data structure.  You shouldn't have to add anything
// to the event loop if you want to compute a new  quantity - rather, you can
// add a new Analyzer
FileReport looper_main(const Configuration& config, const string& filename,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers, long long max_events,
                       long long reportevery);

#endif
