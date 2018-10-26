// In this file, we declare our own custom analyzers, based on the interface
// defined in nanoflow.h
#ifndef MYANALYZERS_H
#define MYANALYZERS_H

#include <TLorentzVector.h>

#include "nanoflow.h"

TLorentzVector make_lv(float pt, float eta, float phi, float mass);

// We can specialize the LazyObject for specific physics objects
// by wrapping the most commonly used branches.
// Jet type based on the on-demand reading of quantities from the underlying
// TTree

class Jet : public LazyObject {
 public:
  const float _pt, _eta, _phi, _mass;
  Jet(NanoEvent* _event, unsigned int _index);
  Jet(Jet&&) = default;
  Jet(const Jet&) = default;
  ~Jet() {}

  inline float pt() const { return _pt; }

  inline float eta() const { return _eta; }

  inline float phi() const { return _phi; }

  inline float mass() const { return _mass; }
};

class GenJet : public LazyObject {
 public:
  const float _pt, _eta, _phi, _mass;
  const int _partonFlavour;
  GenJet(NanoEvent* _event, unsigned int _index);
  ~GenJet() {}

  inline float pt() const { return _pt; }

  inline float eta() const { return _eta; }

  inline float phi() const { return _phi; }

  inline float mass() const { return _mass; }

  inline int partonFlavour() const { return _partonFlavour; }
};

class GenLepton : public LazyObject {
 public:
  const float _pt, _eta, _phi, _mass;
  const int _pdgId;
  GenLepton(NanoEvent* _event, unsigned int _index);
  ~GenLepton() {}

  inline float pt() const { return _pt; }

  inline float eta() const { return _eta; }

  inline float phi() const { return _phi; }

  inline float mass() const { return _mass; }

  inline int pdgId() const { return _pdgId; }
};

class Muon : public LazyObject {
 public:
  float _pt, _eta, _phi, _mass;
  int matchidx = -1;

  Muon(NanoEvent* _event, unsigned int _index);
  ~Muon() {}

  inline float pt() const { return _pt; }

  inline float eta() const { return _eta; }

  inline float phi() const { return _phi; }

  inline float mass() const { return _mass; }
};

// template <class T> void swap (T& a, T& b)
// {
//   T c(std::move(a)); a=std::move(b); b=std::move(c);
// }

class Electron : public LazyObject {
 public:
  const float _pt, _eta, _phi, _mass;
  int matchidx = -1;

  Electron(NanoEvent* _event, unsigned int _index);
  ~Electron() {}

  inline float pt() const { return _pt; }

  inline float eta() const { return _eta; }

  inline float phi() const { return _phi; }

  inline float mass() const { return _mass; }
};

class GenParticle {
 public:
  float _pt, _eta, _phi, _mass;
  int _pdgId;

  GenParticle(float pt, float eta, float phi, float mass, int pdgId)
      : _pt(pt), _eta(eta), _phi(phi), _mass(mass), _pdgId(pdgId) {}
  inline float pt() { return _pt; };
  inline float eta() { return _eta; };
  inline float phi() { return _phi; };
  inline float mass() { return _mass; };
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
// Currently, this is only the input and output files.
// We can load the configuration from a json file
class Configuration {
 public:
  vector<string> input_files;
  string output_filename;

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

  vector<GenParticleInitial> geninitialstate;
  vector<GenParticle> mediators;
  vector<GenParticle> genfinalstatemuon;

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

class MatrixElementEventAnalyzer : public Analyzer {
 public:
  Output& output;
  double sqrt_s;

  MatrixElementEventAnalyzer(Output& _output, double _sqrt_s);
  virtual void analyze(NanoEvent& _event) override;
  virtual const string getName() const override;
  vector<GenParticle> get_particles_idx(Event& event,
                                        vector<unsigned int>& final_mu_idx);
  void match_muons(Event& event, vector<GenParticle>& gen, vector<Muon>& reco);
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
// electrons in the event  and stores it in a histogram. The actual implementation
// code is located in myanalyzers.cc,  so we can compile it separately.
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

  int nGenInitialState;
  array<float, 2> GenInitialState_pz;
  array<float, 2> GenInitialState_energy;
  array<int, 2> GenInitialState_pdgId;

  int nGenMediator;
  array<float, 1> GenMediator_px;
  array<float, 1> GenMediator_py;
  array<float, 1> GenMediator_pz;
  array<float, 1> GenMediator_energy;
  array<int, 1> GenMediator_pdgId;

  int nGenFinalStateMuon;
  array<float, 10> GenFinalStateMuon_px;
  array<float, 10> GenFinalStateMuon_py;
  array<float, 10> GenFinalStateMuon_pz;
  array<float, 10> GenFinalStateMuon_energy;
  array<int, 10> GenFinalStateMuon_pdgId;

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

  void fill_geninitialstate(vector<GenParticleInitial>& src);
  void fill_mediator(vector<GenParticle>& src);
  void fill_genfinalstatemuon(Event& event, vector<GenParticle>& src);
  void fill_muon(Event& event, vector<Muon>& src);
};

// This is the main event loop
// Given a TTreeReader reader, we process all the specified analyzers and store
// the  output in the Output data structure.  You shouldn't have to add anything to
// the event loop if you want to compute a new  quantity - rather, you can add a
// new Analyzer
FileReport looper_main(const Configuration& config, const string& filename,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers,
                       long long max_events);

#endif
