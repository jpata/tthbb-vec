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
class Jet : public LazyObject, public FourMomentumSpherical {
 public:
  Jet(NanoEvent* _event, unsigned int _index);
  Jet(Jet&&) = default;
  Jet(const Jet&) = default;
  ~Jet() {}
};

class GenJet : public LazyObject, public FourMomentumSpherical {
 public:
  const int _partonFlavour;
  GenJet(NanoEvent* _event, unsigned int _index);
  ~GenJet() {}

  inline int partonFlavour() const { return _partonFlavour; }
};

class GenLepton : public LazyObject, public FourMomentumSpherical {
 public:
  const int _pdgId;
  GenLepton(NanoEvent* _event, unsigned int _index);
  ~GenLepton() {}

  inline int pdgId() const { return _pdgId; }
};

class Muon : public LazyObject, public FourMomentumSpherical {
 public:
  int matchidx = -1;

  Muon(NanoEvent* _event, unsigned int _index);
  ~Muon() {}
};

class Electron : public LazyObject, public FourMomentumSpherical {
 public:
  int matchidx = -1;

  Electron(NanoEvent* _event, unsigned int _index);
  ~Electron() {}
};

class GenParticle : public FourMomentumSpherical {
 public:
  int _pdgId;

  GenParticle(float pt, float eta, float phi, float mass, int pdgId)
      : FourMomentumSpherical(pt, eta, phi, mass) {}
  inline int pdgId() { return _pdgId; };
};

class GenParticleInitial {
 public:
  float _pz;
  int _pdgId;

  GenParticleInitial(float pz, int pdgId) : _pz(pz), _pdgId(pdgId) {}
  inline float pz() { return _pz; };
  inline float pdgId() { return _pdgId; };
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
class Event : public NanoEvent {
 public:
  const Configuration& config;

  // We need to predefine the event content here

  // Physics objects
  vector<Jet> jets;
  vector<Muon> muons;
  vector<Electron> electrons;
  vector<GenJet> genjets;
  vector<GenLepton> genleptons;

  // Simple variables
  double lep2_highest_inv_mass;
  int nMuon;
  int nMuon_match;

  Event(TTreeReader& _reader, const Configuration& _config);

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

class GenLeptonEventAnalyzer : public Analyzer {
 public:
  Output& output;

  GenLeptonEventAnalyzer(Output& _output);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

class GenRecoJetPair {
 public:
  float GenJet_pt = 0.0;
  float GenJet_eta = 0.0;
  int GenJet_partonFlavour = 0;
  float Jet_pt = 0.0;
};

class GenRecoJetMatchAnalyzer : public Analyzer {
 public:
  Output& output;
  shared_ptr<TTree> out_tree;
  GenRecoJetPair out_tree_buf;

  GenRecoJetMatchAnalyzer(Output& _output);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

class GenRecoLeptonPair {
 public:
  float GenLepton_pt = 0.0;
  float GenLepton_eta = 0.0;
  int GenLepton_pdgId = 0;
  float Lepton_pt = 0.0;
  int Lepton_pdgId = 0;
};

class GenRecoLeptonMatchAnalyzer : public Analyzer {
 public:
  Output& output;
  shared_ptr<TTree> out_tree;
  GenRecoLeptonPair out_tree_buf;

  GenRecoLeptonMatchAnalyzer(Output& _output);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

// This example Analyzer computes the sum(pt) of all the jets, muons and
// electrons in the event  and stores it in a histogram. The actual
// implementation code is located in myanalyzers.cc,  so we can compile it
// separately.
class SumPtAnalyzer : public Analyzer {
 public:
  Output& output;

  // We store a pointer to the output histogram distribution here in order to
  // address it conveniently  The histogram itself is stored in the Output data
  // structure
  shared_ptr<TH1D> h_sumpt;

  SumPtAnalyzer(Output& _output);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

// This example Analyzer simply saves the number of primary vertices in an
// output histogram
class EventVarsAnalyzer : public Analyzer {
 public:
  Output& output;

  // Convenient short-hand access to the PV histogram
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

// Here we compute as an example the deltaR between all jet pairs
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

  LeptonPairAnalyzer(Output& _output) : output(_output){};

  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
};

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

  void fill_muon(Event& event, vector<Muon>& src);
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
