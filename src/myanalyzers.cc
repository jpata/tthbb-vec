#include "myanalyzers.h"

// We need to write separate constructors for these objects,
// as the branch names will be resolved to branch pointers at compile time
Jet::Jet(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("Jet_pt")),
                            this->get_float(string_hash("Jet_eta")),
                            this->get_float(string_hash("Jet_phi")),
                            this->get_float(string_hash("Jet_mass"))) {}

GenJet::GenJet(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("GenJet_pt")),
                            this->get_float(string_hash("GenJet_eta")),
                            this->get_float(string_hash("GenJet_phi")),
                            this->get_float(string_hash("GenJet_mass"))),
      _partonFlavour(this->get_int(string_hash("GenJet_partonFlavour"))) {}

GenLepton::GenLepton(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("GenPart_pt")),
                            this->get_float(string_hash("GenPart_eta")),
                            this->get_float(string_hash("GenPart_phi")),
                            this->get_float(string_hash("GenPart_mass"))),
      _pdgId(this->get_int(string_hash("GenPart_pdgId"))) {}

Muon::Muon(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("Muon_pt")),
                            this->get_float(string_hash("Muon_eta")),
                            this->get_float(string_hash("Muon_phi")),
                            this->get_float(string_hash("Muon_mass"))),
      matchidx(-1) {}

Electron::Electron(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("Electron_pt")),
                            this->get_float(string_hash("Electron_eta")),
                            this->get_float(string_hash("Electron_phi")),
                            this->get_float(string_hash("Electron_mass"))),
      matchidx(-1) {}


MuonEventAnalyzer::MuonEventAnalyzer(Output& _output) : output(_output) {
  cout << "Creating MuonEventAnalyzer" << endl;
}

void MuonEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

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

const string MuonEventAnalyzer::getName() const { return "MuonEventAnalyzer"; }

ElectronEventAnalyzer::ElectronEventAnalyzer(Output& _output)
    : output(_output) {
  cout << "Creating ElectronEventAnalyzer" << endl;
}

void ElectronEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  event.lc_uint.read(string_hash("nElectron"));
  event.lc_vfloat.read(string_hash("Electron_pt"));
  event.lc_vfloat.read(string_hash("Electron_eta"));
  event.lc_vfloat.read(string_hash("Electron_phi"));
  event.lc_vfloat.read(string_hash("Electron_mass"));

  const auto nElectron = event.lc_uint.get(string_hash("nElectron"));
  for (unsigned int _nElectron = 0; _nElectron < nElectron; _nElectron++) {
    Electron electron(&event, _nElectron);
    event.electrons.push_back(electron);
  }
}

const string ElectronEventAnalyzer::getName() const {
  return "ElectronEventAnalyzer";
}

JetEventAnalyzer::JetEventAnalyzer(Output& _output) : output(_output) {
  cout << "Creating JetEventAnalyzer" << endl;
}

void JetEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  event.lc_uint.read(string_hash("nJet"));
  event.lc_vfloat.read(string_hash("Jet_pt"));
  event.lc_vfloat.read(string_hash("Jet_eta"));
  event.lc_vfloat.read(string_hash("Jet_phi"));
  event.lc_vfloat.read(string_hash("Jet_mass"));

  // Get the number of jets as an uint
  const auto nJet = event.lc_uint.get(string_hash("nJet"));

  // Construct the jet objects from the branches
  for (unsigned int _nJet = 0; _nJet < nJet; _nJet++) {
    Jet jet(&event, _nJet);
    event.jets.push_back(jet);
  }
}

const string JetEventAnalyzer::getName() const { return "JetEventAnalyzer"; }

GenJetEventAnalyzer::GenJetEventAnalyzer(Output& _output) : output(_output) {
  cout << "Creating GenJetEventAnalyzer" << endl;
}

void GenJetEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  // Get the number of jets as an uint
  event.lc_uint.read(string_hash("nGenJet"));
  event.lc_vfloat.read(string_hash("GenJet_pt"));
  event.lc_vfloat.read(string_hash("GenJet_eta"));
  event.lc_vfloat.read(string_hash("GenJet_phi"));
  event.lc_vfloat.read(string_hash("GenJet_mass"));
  event.lc_vint.read(string_hash("GenJet_partonFlavour"));

  const auto nGenJet = event.lc_uint.get(string_hash("nGenJet"));

  // Construct the jet objects from the branches
  for (unsigned int _nGenJet = 0; _nGenJet < nGenJet; _nGenJet++) {
    const auto pt = event.lc_vfloat.get(string_hash("GenJet_pt"), _nGenJet);
    if (pt > 5) {
      GenJet jet(&event, _nGenJet);
      event.genjets.push_back(jet);
    }
  }
}

const string GenJetEventAnalyzer::getName() const {
  return "GenJetEventAnalyzer";
}

GenLeptonEventAnalyzer::GenLeptonEventAnalyzer(Output& _output)
    : output(_output) {
  cout << "Creating GenLeptonEventAnalyzer" << endl;
}

void GenLeptonEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  // Get the number of jets as an uint
  event.lc_uint.read(string_hash("nGenPart"));
  event.lc_vfloat.read(string_hash("GenPart_pt"));
  event.lc_vint.read(string_hash("GenPart_pdgId"));
  event.lc_vint.read(string_hash("GenPart_status"));

  const auto nGenPart = event.lc_uint.get(string_hash("nGenPart"));

  // Construct the jet objects from the branches
  for (unsigned int _nGenPart = 0; _nGenPart < nGenPart; _nGenPart++) {
    const auto pdgId =
        abs(event.lc_vint.get(string_hash("GenPart_pdgId"), _nGenPart));
    const auto status =
        event.lc_vint.get(string_hash("GenPart_status"), _nGenPart);
    const auto pt = event.lc_vfloat.get(string_hash("GenPart_pt"), _nGenPart);

    // electrons or muons
    if ((status == 1 && pt > 5) && ((pdgId == 11) || (pdgId == 13))) {
      event.lc_vfloat.read(string_hash("GenPart_eta"));
      event.lc_vfloat.read(string_hash("GenPart_phi"));
      event.lc_vfloat.read(string_hash("GenPart_mass"));

      GenLepton lep(&event, _nGenPart);
      event.genleptons.push_back(lep);
    }
  }
}

const string GenLeptonEventAnalyzer::getName() const {
  return "GenLeptonEventAnalyzer";
}

GenRecoJetMatchAnalyzer::GenRecoJetMatchAnalyzer(Output& _output)
    : output(_output) {
  cout << "Creating GenRecoJetMatchAnalyzer" << endl;

  output.outfile->cd();
  output.trees[string_hash("GenRecoJetMatch")] =
      std::make_shared<TTree>("GenRecoJetMatch", "GenRecoJetMatch");
  out_tree = output.trees.at(string_hash("GenRecoJetMatch"));

  out_tree->Branch("GenJet_pt", &out_tree_buf.GenJet_pt, "GenJet_pt/F");
  out_tree->Branch("GenJet_eta", &out_tree_buf.GenJet_eta, "GenJet_eta/F");
  out_tree->Branch("GenJet_partonFlavour", &out_tree_buf.GenJet_partonFlavour,
                   "GenJet_partonFlavour/I");
  out_tree->Branch("Jet_pt", &out_tree_buf.Jet_pt, "Jet_pt/F");
}

void GenRecoJetMatchAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  vector<TLorentzVector> lvec_recojet;
  for (unsigned int i = 0; i < event.jets.size(); i++) {
    lvec_recojet.push_back(
        make_lv(event.jets.at(i).pt(), event.jets.at(i).eta(),
                event.jets.at(i).phi(), event.jets.at(i).mass()));
  }

  vector<TLorentzVector> lvec_genjet;
  for (unsigned int i = 0; i < event.genjets.size(); i++) {
    lvec_genjet.push_back(
        make_lv(event.genjets.at(i).pt(), event.genjets.at(i).eta(),
                event.genjets.at(i).phi(), event.genjets.at(i).mass()));
  }

  vector<pair<unsigned int, unsigned int>> gen_reco_matches;

  vector<GenRecoJetPair> results;

  for (unsigned int i = 0; i < lvec_genjet.size(); i++) {
    unsigned int best_match_reco = 0;
    bool found_match = false;
    double min_dr = 1e6;

    for (unsigned int j = 0; j < lvec_recojet.size(); j++) {
      const auto dr = lvec_genjet.at(i).DeltaR(lvec_recojet.at(j));
      if ((dr < 0.3) && (dr < min_dr)) {
        best_match_reco = j;
        min_dr = dr;
        found_match = true;
      }
    }

    GenRecoJetPair p;
    p.GenJet_pt = lvec_genjet.at(i).Pt();
    p.GenJet_eta = lvec_genjet.at(i).Eta();
    p.GenJet_partonFlavour = event.genjets.at(i).partonFlavour();

    if (found_match) {
      gen_reco_matches.push_back(make_pair(i, best_match_reco));
      p.Jet_pt = lvec_recojet.at(best_match_reco).Pt();
    }
    results.push_back(p);
  }
  for (auto& r : results) {
    out_tree_buf = r;
    out_tree->Fill();
  }
}

const string GenRecoJetMatchAnalyzer::getName() const {
  return "GenRecoJetMatchAnalyzer";
}

GenRecoLeptonMatchAnalyzer::GenRecoLeptonMatchAnalyzer(Output& _output)
    : output(_output) {
  cout << "Creating GenRecoLeptonMatchAnalyzer" << endl;

  output.outfile->cd();
  output.trees[string_hash("GenRecoLeptonMatch")] =
      std::make_shared<TTree>("GenRecoLeptonMatch", "GenRecoLeptonMatch");
  out_tree = output.trees.at(string_hash("GenRecoLeptonMatch"));

  out_tree->Branch("GenLepton_pt", &out_tree_buf.GenLepton_pt,
                   "GenLepton_pt/F");
  out_tree->Branch("GenLepton_eta", &out_tree_buf.GenLepton_eta,
                   "GenLepton_eta/F");
  out_tree->Branch("GenLepton_pdgId", &out_tree_buf.GenLepton_pdgId,
                   "GenLepton_pdgId/I");
  out_tree->Branch("Lepton_pt", &out_tree_buf.Lepton_pt, "Lepton_pt/F");
}

void GenRecoLeptonMatchAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  vector<TLorentzVector> lvec_recomu;
  for (unsigned int i = 0; i < event.muons.size(); i++) {
    lvec_recomu.push_back(
        make_lv(event.muons.at(i).pt(), event.muons.at(i).eta(),
                event.muons.at(i).phi(), event.muons.at(i).mass()));
  }

  vector<TLorentzVector> lvec_recoel;
  for (unsigned int i = 0; i < event.electrons.size(); i++) {
    lvec_recoel.push_back(
        make_lv(event.electrons.at(i).pt(), event.electrons.at(i).eta(),
                event.electrons.at(i).phi(), event.electrons.at(i).mass()));
  }

  vector<TLorentzVector> lvec_genlep;
  for (unsigned int i = 0; i < event.genleptons.size(); i++) {
    lvec_genlep.push_back(
        make_lv(event.genleptons.at(i).pt(), event.genleptons.at(i).eta(),
                event.genleptons.at(i).phi(), event.genleptons.at(i).mass()));
  }

  vector<pair<unsigned int, unsigned int>> gen_reco_matches;

  vector<GenRecoLeptonPair> results;

  for (unsigned int i = 0; i < lvec_genlep.size(); i++) {
    unsigned int best_match_reco = 0;
    const auto genlep_pdgId = event.genleptons.at(i).pdgId();
    bool found_match = false;
    double min_dr = 99.0;

    vector<TLorentzVector>* lvec_recolep;
    if (genlep_pdgId == 11) {
      lvec_recolep = &lvec_recoel;
    } else if (genlep_pdgId == 13) {
      lvec_recolep = &lvec_recomu;
    } else {
      continue;
    }

    for (unsigned int j = 0; j < lvec_recolep->size(); j++) {
      const auto dr = lvec_genlep.at(i).DeltaR(lvec_recolep->at(j));
      if ((dr < 0.3) && (dr < min_dr)) {
        best_match_reco = j;
        min_dr = dr;
        found_match = true;
      }
    }

    GenRecoLeptonPair p;
    p.GenLepton_pt = lvec_genlep.at(i).Pt();
    p.GenLepton_eta = lvec_genlep.at(i).Eta();
    p.GenLepton_pdgId = event.genleptons.at(i).pdgId();

    if (found_match) {
      gen_reco_matches.push_back(make_pair(i, best_match_reco));
      p.Lepton_pt = lvec_recolep->at(best_match_reco).Pt();

      if (genlep_pdgId == 11) {
        event.electrons.at(best_match_reco).matchidx = (int)i;
      } else if (genlep_pdgId == 13) {
        event.muons.at(best_match_reco).matchidx = (int)i;
      }
    }
    results.push_back(p);
  }
  for (auto& r : results) {
    out_tree_buf = r;
    out_tree->Fill();
  }
}

const string GenRecoLeptonMatchAnalyzer::getName() const {
  return "GenRecoLeptonMatchAnalyzer";
}

Configuration::Configuration(const std::string& json_file) {
  ifstream inp(json_file);
  json input_json;
  inp >> input_json;

  for (auto fn : input_json.at("input_filenames")) {
    input_files.push_back(fn);
  }
  output_filename = input_json.at("output_filename").get<std::string>();
}

Event::Event(TTreeReader& _reader, const Configuration& _config)
    : NanoEvent(_reader), config(_config) {}

// This is very important to make sure that we always start with a clean
// event and we don't keep any information from previous events
void Event::clear_event() {
  jets.clear();
  muons.clear();
  electrons.clear();
  genjets.clear();
  genleptons.clear();

  lep2_highest_inv_mass = 0;
  nMuon = 0;
  nMuon_match = 0;
}

// In this function we create our event representation
// In order to have a fast runtime, we need to do the
// absolute minimum here.
void Event::analyze() {
  clear_event();

  this->lc_uint.read(string_hash("run"));
  this->lc_uint.read(string_hash("luminosityBlock"));
  this->lc_ulong64.read(string_hash("event"));

  this->run = this->lc_uint.get(string_hash("run"));
  this->luminosityBlock = this->lc_uint.get(string_hash("luminosityBlock"));
  this->event = this->lc_ulong64.get(string_hash("event"));
}

SumPtAnalyzer::SumPtAnalyzer(Output& _output) : output(_output) {
  cout << "Creating SumPtAnalyzer" << endl;
  // Create the sumpt histogram in the output data structure
  output.outfile->cd();
  output.histograms_1d[string_hash("h_sumpt")] =
      make_shared<TH1D>("h_sumpt", "sumpt", 100, 0, 1000);

  // Retrieve the pointer to the sumpt histogram for convenient access
  h_sumpt = output.histograms_1d.at(string_hash("h_sumpt"));
}

void SumPtAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);
  double sum_pt = 0.0;

  for (auto& jet : event.jets) {
    sum_pt += jet.pt();
  }
  for (auto& muon : event.muons) {
    sum_pt += muon.pt();
  }
  for (auto& electron : event.electrons) {
    sum_pt += electron.pt();
  }
  h_sumpt->Fill(sum_pt);
}

const string SumPtAnalyzer::getName() const { return "SumPtAnalyzer"; }

EventVarsAnalyzer::EventVarsAnalyzer(Output& _output) : output(_output) {
  cout << "Creating EventVarsAnalyzer" << endl;

  // Create and store the nPV histogram in the Output structure
  output.outfile->cd();
  output.histograms_1d[string_hash("h_nPVs")] =
      make_shared<TH1D>("h_nPVs", "nPVs", 50, 0, 50);

  h_nPVs = output.histograms_1d.at(string_hash("h_nPVs"));
}

void EventVarsAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);
  event.lc_int.read(string_hash("PV_npvsGood"));
  h_nPVs->Fill(event.lc_int.get(string_hash("PV_npvsGood")));
}
const string EventVarsAnalyzer::getName() const { return "EventVarsAnalyzer"; }

JetJetPair::JetJetPair(const Jet& _j1, const Jet& _j2, const double _dr)
    : j1(&_j1), j2(&_j2), dr(_dr) {}

JetDeltaRAnalyzer::JetDeltaRAnalyzer(Output& _output) : output(_output) {
  cout << "Creating JetDeltaRAnalyzer" << endl;
  output.outfile->cd();
  output.histograms_1d[string_hash("h_deltaR")] =
      make_shared<TH1D>("h_deltaR", "Jet pair dR", 100, 0, 6);

  h_deltaR = output.histograms_1d.at(string_hash("h_deltaR"));
}

void JetDeltaRAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);
  const auto njets = event.jets.size();

  // we would like to store a list of all the jet pairs along with their delta R
  // This allows us to find, for example, the two closest jets.
  vector<JetJetPair> jet_drpairs;

  // Creating the TLorentzVector is math-heavy, so we do it only once outside
  // the jet-jet loop
  vector<TLorentzVector> jet_lvs;
  for (auto jet : event.jets) {
    jet_lvs.push_back(make_lv(jet.pt(), jet.eta(), jet.phi(), jet.mass()));
  }

  // Loop over jet pairs
  for (unsigned int i = 0; i < njets; i++) {
    for (unsigned int j = i + 1; j < njets; j++) {
      const auto& lv1 = jet_lvs.at(i);
      const auto& lv2 = jet_lvs.at(j);

      const auto dr = lv1.DeltaR(lv2);
      jet_drpairs.push_back(JetJetPair(event.jets.at(i), event.jets.at(j), dr));
      h_deltaR->Fill(dr);
    }
  }
}

const string JetDeltaRAnalyzer::getName() const { return "JetDeltaRAnalyzer"; }

void LeptonPairAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  vector<FourMomentumSpherical*> leps;
  for (const auto& lep : event.muons) {
    leps.push_back((FourMomentumSpherical*)&lep);
  }
  for (const auto& lep : event.electrons) {
    leps.push_back((FourMomentumSpherical*)&lep);
  }

  vector<TLorentzVector> lep_lvs;
  for (const auto* lep : leps) {
    lep_lvs.push_back(make_lv(lep->pt(), lep->eta(), lep->phi(), lep->mass()));
  }

  double lep2_highest_inv_mass = 0;
  for (auto il1 = 0; il1 < leps.size(); il1++) {
    for (auto il2 = il1 + 1; il2 < leps.size(); il2++) {
      const auto& lv1 = lep_lvs.at(il1);
      const auto& lv2 = lep_lvs.at(il2);

      const auto& lv_tot = lv1 + lv2;
      const auto inv_mass = lv_tot.M();

      if (inv_mass > lep2_highest_inv_mass) {
        lep2_highest_inv_mass = inv_mass;
      }
    }
  }
  event.lep2_highest_inv_mass = lep2_highest_inv_mass;
};

const string LeptonPairAnalyzer::getName() const {
  return "LeptonPairAnalyzer";
};

MyTreeAnalyzer::MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {
  out_tree->Branch("lep2_highest_inv_mass", &lep2_highest_inv_mass,
                   "lep2_highest_inv_mass/F");

  out_tree->Branch("nMuon_match", &nMuon_match, "nMuon_match/I");

  out_tree->Branch("nMuon", &nMuon, "nMuon/I");
  out_tree->Branch("Muon_px", Muon_px.data(), "Muon_px[nMuon]/F");
  out_tree->Branch("Muon_py", Muon_py.data(), "Muon_py[nMuon]/F");
  out_tree->Branch("Muon_pz", Muon_pz.data(), "Muon_pz[nMuon]/F");
  out_tree->Branch("Muon_energy", Muon_energy.data(), "Muon_energy[nMuon]/F");
  out_tree->Branch("Muon_matchidx", Muon_matchidx.data(),
                   "Muon_matchidx[nMuon]/I");
}

void MyTreeAnalyzer::clear() {
  lep2_highest_inv_mass = 0.0;
  nMuon = 0;
  nMuon_match = 0;

  nMuon = 0;
  Muon_px.fill(0.0);
  Muon_py.fill(0.0);
  Muon_pz.fill(0.0);
  Muon_energy.fill(0.0);
  Muon_matchidx.fill(0);
}

void MyTreeAnalyzer::fill_muon(Event& event, vector<Muon>& src) {
  if (src.size() > Muon_px.size()) {
    cerr << "event " << event.event << " " << src.size() << endl;
  }
  nMuon = static_cast<int>(src.size());

  unsigned int i = 0;
  for (auto& gp : src) {
    if (i >= Muon_px.size()) {
      cerr << "ERROR: Muon out of range" << endl;
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

void MyTreeAnalyzer::analyze(NanoEvent& _event) {
  this->clear();

  auto& event = static_cast<Event&>(_event);
  lep2_highest_inv_mass = event.lep2_highest_inv_mass;
  nMuon = event.nMuon;
  nMuon_match = event.nMuon_match;

  fill_muon(event, event.muons);

  TreeAnalyzer::analyze(event);
}

const string MyTreeAnalyzer::getName() const { return "MyTreeAnalyzer"; }

FileReport looper_main(const Configuration& config, const string& filename,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers, long long max_events,
                       long long reportevery) {
  // Make sure we clear the state of the reader
  reader.Restart();

  TStopwatch sw;
  sw.Start();

  // We initialize the C++ representation of the event (data row) from the
  // TTreeReader
  Event event(reader, config);

  // Keep track of the number of events we processed
  unsigned long long nevents = 0;

  // Keep track of the total time per event
  FileReport report(filename, analyzers);

  // Start the loop over the TTree events
  cout << "starting loop over " << reader.GetEntries(true)
       << " events in TTree " << reader.GetTree() << endl;
  while (reader.Next()) {
    // In case of early termination
    if (max_events > 0 && nevents == max_events) {
      cout << "breaking event loop before event " << nevents << endl;
      break;
    }

    auto time_t0 = std::chrono::high_resolution_clock::now();

    // We initialize the event
    event.analyze();

    auto time_t1 = std::chrono::high_resolution_clock::now();
    auto time_dt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0)
            .count();
    report.event_duration += time_dt;

    unsigned int iAnalyzer = 0;

    // We run all the analyzers one after the other
    for (auto* analyzer : analyzers) {
      auto time_t0 = std::chrono::high_resolution_clock::now();

      // Here we do the actual work for the analyzer
      analyzer->analyze(event);

      // Get the time in nanoseconds spent per event for this analyzer
      auto time_t1 = std::chrono::high_resolution_clock::now();
      auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         time_t1 - time_t0)
                         .count();
      report.analyzer_durations[iAnalyzer] += time_dt;

      iAnalyzer += 1;
    }

    // Print out a progress report
    if (nevents % reportevery == 0) {
      const auto elapsed_time = sw.RealTime();
      const auto speed = nevents / elapsed_time;
      const auto remaining_events = (reader.GetEntries(true) - nevents);
      const auto remaining_time = remaining_events / speed;

      cout << "Processed " << nevents << "/" << reader.GetEntries(true)
           << " speed=" << speed / 1000.0 << "kHz ETA=" << remaining_time << "s"
           << endl;
      sw.Continue();
    }
    nevents += 1;
  }
  report.num_events_processed = nevents;

  sw.Stop();

  // Print out some statistics
  report.cpu_time = sw.CpuTime();
  report.real_time = sw.RealTime();

  // Compute the event processing speed in kHz
  report.speed =
      (double)report.num_events_processed / report.real_time / 1000.0;

  report.print(cout);

  cout << "looper_main"
       << " nevents=" << report.num_events_processed
       << ",cpu_time=" << report.cpu_time << ",real_time=" << report.real_time
       << ",speed=" << report.speed << endl;

  return report;
}
