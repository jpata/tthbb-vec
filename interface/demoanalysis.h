#include "nanoflow.h"

using namespace nanoflow;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                             PHYSICS OBJECTS                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//Objects that derive from this class contain the spherical components of
//four-momentum and getter functions for those.
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


// We create a Muon object which has spherical 4-momentum components
// It also derives from the nanoflow::LazyObject, which allows the
// muon data to be populated easily from the TTree branches. 
class Muon : public LazyObject, public FourMomentumSpherical {
 public:

  //Index of the matched generator muon
  int matchidx = -1;

  Muon(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("Muon_pt")),
                            this->get_float(string_hash("Muon_eta")),
                            this->get_float(string_hash("Muon_phi")),
                            this->get_float(string_hash("Muon_mass"))),
      matchidx(-1) {}

  ~Muon() {}
};

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                             EVENT STRUCTURE                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


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

  MyAnalysisEvent(TTreeReader& _reader, const Configuration& _config)
    : NanoEvent(_reader), config(_config) {}

  // This is very important to make sure that we always start with a clean
  // event and we don't keep any information from previous events
  void clear_event() {
    muons.clear();

    nMuon = 0;
  }

  // In this function we create our event representation
  // In order to have a fast runtime, we need to do the
  // absolute minimum here.
  void analyze() {
    clear_event();

    //In older NanoAOD, this was int instead of uint
    //this->lc_uint.read(string_hash("run"));
    this->lc_int.read(string_hash("run"));
    //this->run = this->lc_uint.get(string_hash("run"));
    this->run = this->lc_int.get(string_hash("run"));
    
    this->lc_uint.read(string_hash("luminosityBlock"));
    this->luminosityBlock = this->lc_uint.get(string_hash("luminosityBlock"));
    this->lc_ulong64.read(string_hash("event"));
    this->event = this->lc_ulong64.get(string_hash("event"));
  }
};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                                ANALYZERS                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//Loads muons from the underlying TTree
class MuonEventAnalyzer : public Analyzer {
 public:
  Output& output;

  MuonEventAnalyzer(Output& _output) : output(_output) {
    cout << "Creating MuonEventAnalyzer" << endl;
  }

  virtual void analyze(NanoEvent& _event) override {
    auto& event = static_cast<MyAnalysisEvent&>(_event);

    // Read the necessary branches from disk
    event.lc_uint.read(string_hash("nMuon"));
    event.lc_vfloat.read(string_hash("Muon_pt"));
    event.lc_vfloat.read(string_hash("Muon_eta"));
    event.lc_vfloat.read(string_hash("Muon_phi"));
    event.lc_vfloat.read(string_hash("Muon_mass"));

    // Construct muon objects from the branches and put them to the event
    const auto nMuon = event.lc_uint.get(string_hash("nMuon"));
    for (unsigned int _nMuon = 0; _nMuon < nMuon; _nMuon++) {
      Muon muon(&event, _nMuon);
      event.muons.push_back(muon);
    }
  }

  virtual const string getName() const override { return "MuonEventAnalyzer"; }

};


///////////////////////////////////////////////////////////////////////////////
//                                                                           //                           
//                               OUTPUT TREE                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

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

  MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {

    out_tree->Branch("nMuon", &nMuon, "nMuon/I");
    out_tree->Branch("Muon_px", Muon_px.data(), "Muon_px[nMuon]/F");
    out_tree->Branch("Muon_py", Muon_py.data(), "Muon_py[nMuon]/F");
    out_tree->Branch("Muon_pz", Muon_pz.data(), "Muon_pz[nMuon]/F");
    out_tree->Branch("Muon_energy", Muon_energy.data(), "Muon_energy[nMuon]/F");
    out_tree->Branch("Muon_matchidx", Muon_matchidx.data(),
                     "Muon_matchidx[nMuon]/I");
  }

  virtual const string getName() const override { return "MyTreeAnalyzer"; }

  void clear() {
    nMuon = 0;

    nMuon = 0;
    Muon_px.fill(0.0);
    Muon_py.fill(0.0);
    Muon_pz.fill(0.0);
    Muon_energy.fill(0.0);
    Muon_matchidx.fill(0);
  }

  void fill_muon(MyAnalysisEvent& event, vector<Muon>& src) {
    nMuon = static_cast<int>(src.size());

    unsigned int i = 0;
    for (auto& gp : src) {
      if (i >= Muon_px.size()) {
        cerr << "ERROR: fill_muon, Muon out of range: " << src.size() << ">=" << Muon_px.size() << " event " << event.event << " " << src.size() << endl;
        break;
      }
      const auto lv = make_lv(gp.pt(), gp.eta(), gp.phi(), gp.mass());
      Muon_px[i] = static_cast<float>(lv.Px());
      Muon_py[i] = static_cast<float>(lv.Py());
      Muon_pz[i] = static_cast<float>(lv.Pz());
      Muon_energy[i] = static_cast<float>(lv.Energy());
      Muon_matchidx[i] = src.at(i).matchidx;
      i += 1;
    }
  }

  virtual void analyze(NanoEvent& _event) override {
    this->clear();

    auto& event = static_cast<MyAnalysisEvent&>(_event);
    nMuon = event.nMuon;

    fill_muon(event, event.muons);

    TreeAnalyzer::analyze(event);
  }
};

//Define the looper function with specified templates
static inline FileReport looper_main_demoanalysis(const Configuration& config,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers) {
  return looper_main<MyAnalysisEvent, Configuration>(config, reader, output, analyzers);
};
