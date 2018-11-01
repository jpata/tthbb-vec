#include "myanalyzers.h"

MatrixElementEventAnalyzer::MatrixElementEventAnalyzer(Output& _output,
                                                       double _sqrt_s, string mg_card_path)
    : output(_output), sqrt_s(_sqrt_s), memcalc(mg_card_path) {
  cout << "Creating MatrixElementEventAnalyzer" << endl;

}

void MatrixElementEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<Event&>(_event);

  event.lc_uint.read(string_hash("nGenPart"));
  event.lc_vint.read(string_hash("GenPart_pdgId"));
  event.lc_vint.read(string_hash("GenPart_status"));
  event.lc_vint.read(string_hash("GenPart_genPartIdxMother"));
  event.lc_vfloat.read(string_hash("GenPart_pt"));
  event.lc_vfloat.read(string_hash("GenPart_eta"));
  event.lc_vfloat.read(string_hash("GenPart_phi"));
  event.lc_vfloat.read(string_hash("GenPart_mass"));
  event.lc_float.read(string_hash("Generator_x1"));
  event.lc_float.read(string_hash("Generator_x2"));

  // reconstruct initial state
  const auto x1 = event.lc_float.get(string_hash("Generator_x1"));
  const auto x2 = event.lc_float.get(string_hash("Generator_x2"));

  const auto E1 = sqrt_s * x1 / 2.0;
  const auto E2 = sqrt_s * x2 / 2.0;
  const auto pz1 = E1;
  const auto pz2 = -E2;
  vector<int> initial_pdgId;

  vector<unsigned int> mediator_idx;
  vector<unsigned int> final_mu_idx;

  const auto nGenPart = event.lc_uint.get(string_hash("nGenPart"));
  for (unsigned int _nGenPart = 0; _nGenPart < nGenPart; _nGenPart++) {
    const auto status =
        event.lc_vint.get(string_hash("GenPart_status"), _nGenPart);
    const auto pdgId =
        event.lc_vint.get(string_hash("GenPart_pdgId"), _nGenPart);
    // const auto pt = event.lc_vfloat.get(string_hash("GenPart_pt"),
    // _nGenPart);

    // Initial state
    if (status == 21) {
      initial_pdgId.push_back(pdgId);

      // intermediate or mediator particle
    } else if ((status == 22 && (pdgId == 25 || pdgId == 23))) {
      mediator_idx.push_back(_nGenPart);
      // final state particle
    } else if (status == 1) {
      const auto mother_idx =
          event.lc_vint.get(string_hash("GenPart_genPartIdxMother"), _nGenPart);
      if (mother_idx >= 0 && mother_idx < nGenPart) {
        const auto mother_pdgId =
            event.lc_vint.get(string_hash("GenPart_pdgId"), mother_idx);

        if (mother_pdgId == 25 || mother_pdgId == 23) {
          final_mu_idx.push_back(_nGenPart);
        }
      }
    }
  }

  assert(initial_pdgId.size() == 2);

  event.geninitialstate.push_back(GenParticleInitial(pz1, initial_pdgId.at(0)));
  event.geninitialstate.push_back(GenParticleInitial(pz2, initial_pdgId.at(1)));

  TLorentzVector i1_gen(0, 0, pz1, E1);
  TLorentzVector i2_gen(0, 0, pz2, E2);

  event.mediators = get_particles_idx(event, mediator_idx);

  event.genfinalstatemuon = get_particles_idx(event, final_mu_idx);
  match_muons(event, event.genfinalstatemuon, event.muons);

  if (event.geninitialstate.size() == 2 && event.genfinalstatemuon.size() == 2) {
    
    TLorentzVector mu1 = make_lv(event.genfinalstatemuon.at(0).pt(), event.genfinalstatemuon.at(0).eta(), event.genfinalstatemuon.at(0).phi(), event.genfinalstatemuon.at(0).mass());
    TLorentzVector mu2 = make_lv(event.genfinalstatemuon.at(1).pt(), event.genfinalstatemuon.at(1).eta(), event.genfinalstatemuon.at(1).phi(), event.genfinalstatemuon.at(1).mass());

    auto total_fs = mu1+mu2;
    auto boost_beta = -TVector3(total_fs.Px()/total_fs.E(), total_fs.Py()/total_fs.E(), 0.0);
    mu1.Boost(boost_beta);
    mu2.Boost(boost_beta);

    auto total = i1_gen + i2_gen - (mu1 + mu2);

    const auto phase_space_point = memcalc.make_phase_space_4_lv(i1_gen, i2_gen, mu1, mu2);

    event.me_gen_sig = memcalc.compute_aplitude_gghmumu(phase_space_point);
    event.me_gen_bkg = memcalc.compute_aplitude_qqZmumu(phase_space_point);
  }

   if (event.muons.size() >= 2) {
     auto mu1 = make_lv(event.muons.at(0).pt(), event.muons.at(0).eta(), event.muons.at(0).phi(), event.muons.at(0).mass());
     auto mu2 = make_lv(event.muons.at(1).pt(), event.muons.at(1).eta(), event.muons.at(1).phi(), event.muons.at(1).mass());
     auto total_fs = mu1+mu2;
     auto boost_beta = -TVector3(total_fs.Px()/total_fs.E(), total_fs.Py()/total_fs.E(), 0.0);
     mu1.Boost(boost_beta);
     mu2.Boost(boost_beta);
     total_fs = mu1+mu2;

     const auto E = total_fs.E();
     const auto pz = total_fs.Pz();
     TLorentzVector i1_reco(0, 0, (E+pz)/2.0, (E+pz)/2.0);
     TLorentzVector i2_reco(0, 0, -(E-pz)/2.0, (E-pz)/2.0);

     const auto phase_space_point = memcalc.make_phase_space_4_lv(i1_reco, i2_reco, mu1, mu2);

     event.me_reco_sig = memcalc.compute_aplitude_gghmumu(phase_space_point);
     event.me_reco_bkg = memcalc.compute_aplitude_qqZmumu(phase_space_point);
   }
}

vector<GenParticle> MatrixElementEventAnalyzer::get_particles_idx(
    Event& event, vector<unsigned int>& gen_idx) {
  vector<GenParticle> ret;
  for (auto idx : gen_idx) {
    GenParticle gp(event.lc_vfloat.get(string_hash("GenPart_pt"), idx),
                   event.lc_vfloat.get(string_hash("GenPart_eta"), idx),
                   event.lc_vfloat.get(string_hash("GenPart_phi"), idx),
                   event.lc_vfloat.get(string_hash("GenPart_mass"), idx),
                   event.lc_vint.get(string_hash("GenPart_pdgId"), idx));
    ret.push_back(gp);
  }
  return ret;
}

void MatrixElementEventAnalyzer::match_muons(Event& event,
                                             vector<GenParticle>& gen,
                                             vector<Muon>& reco) {
  vector<TLorentzVector> lvec_recolep;
  vector<TLorentzVector> lvec_genlep;

  for (unsigned int i = 0; i < reco.size(); i++) {
    lvec_recolep.push_back(make_lv(reco.at(i).pt(), reco.at(i).eta(),
                                   reco.at(i).phi(), reco.at(i).mass()));
  }

  for (unsigned int i = 0; i < gen.size(); i++) {
    lvec_genlep.push_back(make_lv(gen.at(i).pt(), gen.at(i).eta(),
                                  gen.at(i).phi(), gen.at(i).mass()));
  }

  for (unsigned int i = 0; i < lvec_genlep.size(); i++) {
    double min_dr = 99.0;
    unsigned int best_match_reco = 0;
    bool found_match = false;

    for (unsigned int j = 0; j < lvec_recolep.size(); j++) {
      const auto dr = lvec_genlep.at(i).DeltaR(lvec_recolep.at(j));
      if ((dr < 0.3) && (dr < min_dr)) {
        best_match_reco = j;
        min_dr = dr;
        found_match = true;
      }
    }
    if (found_match) {
      reco.at(best_match_reco).matchidx = static_cast<int>(i);
    }
  }

  count_matched_mu(event, reco);
}

void MatrixElementEventAnalyzer::count_matched_mu(Event& event,
                                                  vector<Muon>& reco) {
  for (auto& p : reco) {
    event.nMuon += 1;
    if (p.matchidx != -1) {
      event.nMuon_match += 1;
    }
  }
}

const string MatrixElementEventAnalyzer::getName() const {
  return "MatrixElementEventAnalyzer";
}

MyTreeAnalyzer::MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {
  out_tree->Branch("lep2_highest_inv_mass", &lep2_highest_inv_mass,
                   "lep2_highest_inv_mass/F");

  out_tree->Branch("me_gen_sig", &me_gen_sig, "me_gen_sig/D");
  out_tree->Branch("me_gen_bkg", &me_gen_bkg, "me_gen_bkg/D");
  out_tree->Branch("me_reco_sig", &me_reco_sig, "me_reco_sig/D");
  out_tree->Branch("me_reco_bkg", &me_reco_bkg, "me_reco_bkg/D");

  out_tree->Branch("nMuon", &nMuon, "nMuon/I");
  out_tree->Branch("nMuon_match", &nMuon_match, "nMuon_match/I");
  out_tree->Branch("nGenInitialState", &nGenInitialState, "nGenInitialState/I");
  out_tree->Branch("GenInitialState_pz", GenInitialState_pz.data(),
                   "GenInitialState_pz[nGenInitialState]/F");
  out_tree->Branch("GenInitialState_energy", GenInitialState_energy.data(),
                   "GenInitialState_energy[nGenInitialState]/F");
  out_tree->Branch("GenInitialState_pdgId", GenInitialState_pdgId.data(),
                   "GenInitialState_pdgId[nGenInitialState]/I");

  out_tree->Branch("nGenMediator", &nGenMediator, "nGenMediator/I");
  out_tree->Branch("GenMediator_px", GenMediator_px.data(),
                   "GenMediator_px[nGenMediator]/F");
  out_tree->Branch("GenMediator_py", GenMediator_py.data(),
                   "GenMediator_py[nGenMediator]/F");
  out_tree->Branch("GenMediator_pz", GenMediator_pz.data(),
                   "GenMediator_pz[nGenMediator]/F");
  out_tree->Branch("GenMediator_energy", GenMediator_energy.data(),
                   "GenMediator_energy[nGenMediator]/F");
  out_tree->Branch("GenMediator_pdgId", GenMediator_pdgId.data(),
                   "GenMediator_pdgId[nGenMediator]/I");

  out_tree->Branch("nGenFinalStateMuon", &nGenFinalStateMuon,
                   "nGenFinalStateMuon/I");
  out_tree->Branch("GenFinalStateMuon_px", GenFinalStateMuon_px.data(),
                   "GenFinalStateMuon_px[nGenFinalStateMuon]/F");
  out_tree->Branch("GenFinalStateMuon_py", GenFinalStateMuon_py.data(),
                   "GenFinalStateMuon_py[nGenFinalStateMuon]/F");
  out_tree->Branch("GenFinalStateMuon_pz", GenFinalStateMuon_pz.data(),
                   "GenFinalStateMuon_pz[nGenFinalStateMuon]/F");
  out_tree->Branch("GenFinalStateMuon_energy", GenFinalStateMuon_energy.data(),
                   "GenFinalStateMuon_energy[nGenFinalStateMuon]/F");
  out_tree->Branch("GenFinalStateMuon_pdgId", GenFinalStateMuon_pdgId.data(),
                   "GenFinalStateMuon_pdgId[nGenFinalStateMuon]/I");

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

  me_gen_sig = 0.0;
  me_gen_bkg = 0.0;

  nGenInitialState = 0;
  GenInitialState_pz.fill(0.0);
  GenInitialState_pdgId.fill(0);

  nGenMediator = 0;
  GenMediator_px.fill(0.0);
  GenMediator_py.fill(0.0);
  GenMediator_pz.fill(0.0);
  GenMediator_energy.fill(0.0);
  GenMediator_pdgId.fill(0);

  nGenFinalStateMuon = 0;
  GenFinalStateMuon_px.fill(0.0);
  GenFinalStateMuon_py.fill(0.0);
  GenFinalStateMuon_pz.fill(0.0);
  GenFinalStateMuon_energy.fill(0.0);
  GenFinalStateMuon_pdgId.fill(0);

  nMuon = 0;
  Muon_px.fill(0.0);
  Muon_py.fill(0.0);
  Muon_pz.fill(0.0);
  Muon_energy.fill(0.0);
  Muon_matchidx.fill(0);
}

void MyTreeAnalyzer::fill_geninitialstate(vector<GenParticleInitial>& src) {
  assert(src.size() <= GenInitialState_pz.size());
  nGenInitialState = static_cast<int>(src.size());

  unsigned int i = 0;
  for (auto& gp : src) {
    GenInitialState_pz[i] = gp.pz();
    GenInitialState_pdgId[i] = gp.pdgId();
    i += 1;
  }
}

void MyTreeAnalyzer::fill_mediator(vector<GenParticle>& src) {
  assert(src.size() <= GenMediator_px.size());
  nGenMediator = static_cast<int>(src.size());

  unsigned int i = 0;
  for (auto& gp : src) {
    const auto lv = make_lv(gp.pt(), gp.eta(), gp.phi(), gp.mass());
    GenMediator_px[i] = static_cast<float>(lv.Px());
    GenMediator_py[i] = static_cast<float>(lv.Py());
    GenMediator_pz[i] = static_cast<float>(lv.Pz());
    GenMediator_energy[i] = static_cast<float>(lv.Energy());
    GenMediator_pdgId[i] = gp.pdgId();
    i += 1;
  }
}

void MyTreeAnalyzer::fill_genfinalstatemuon(Event& event,
                                            vector<GenParticle>& src) {
  if (src.size() > GenFinalStateMuon_px.size()) {
    cerr << "event " << event.event << " " << src.size() << endl;
  }
  nGenFinalStateMuon = static_cast<int>(src.size());

  unsigned int i = 0;
  for (auto& gp : src) {
    if (i >= GenFinalStateMuon_px.size()) {
      cerr << "ERROR: GenFinalState out of range" << endl;
      break;
    }
    const auto lv = make_lv(gp.pt(), gp.eta(), gp.phi(), gp.mass());
    GenFinalStateMuon_px[i] = static_cast<float>(lv.Px());
    GenFinalStateMuon_py[i] = static_cast<float>(lv.Py());
    GenFinalStateMuon_pz[i] = static_cast<float>(lv.Pz());
    GenFinalStateMuon_energy[i] = static_cast<float>(lv.Energy());
    GenFinalStateMuon_pdgId[i] = gp.pdgId();
    i += 1;
  }
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

  me_gen_sig = event.me_gen_sig;
  me_gen_bkg = event.me_gen_bkg;

  me_reco_sig = event.me_reco_sig;
  me_reco_bkg = event.me_reco_bkg;

  fill_geninitialstate(event.geninitialstate);
  fill_mediator(event.mediators);
  fill_genfinalstatemuon(event, event.genfinalstatemuon);
  fill_muon(event, event.muons);

  TreeAnalyzer::analyze(event);
}

const string MyTreeAnalyzer::getName() const { return "MyTreeAnalyzer"; }
