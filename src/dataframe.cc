#include <iostream>
#include <map>
#include <string>

#include <TFile.h>
#include <TLorentzVector.h>
#include <TROOT.h>
#include <ROOT/RDataFrame.hxx>

using namespace std;
typedef ROOT::VecOps::RVec<float> vfloat;

TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
  TLorentzVector lv;
  lv.SetPtEtaPhiM(pt, eta, phi, mass);
  return lv;
}

bool IsGoodEvent(unsigned int nJet, vfloat Jet_pt, vfloat Jet_eta,
                 vfloat Jet_phi, vfloat Jet_mass,

                 unsigned int nGenJet, vfloat GenJet_pt, vfloat GenJet_eta,
                 vfloat GenJet_phi, vfloat GenJet_mass) {
  return (nJet > 0) && (nGenJet > 0);
}

class GenJetRecoJetPair {
 public:
  float GenJet_pt = 0.0;
  float GenJet_eta = 0.0;
  float RecoJet_pt = 0.0;
};

vector<GenJetRecoJetPair> DoStuff(unsigned int nJet, vfloat Jet_pt,
                                  vfloat Jet_eta, vfloat Jet_phi,
                                  vfloat Jet_mass,

                                  unsigned int nGenJet, vfloat GenJet_pt,
                                  vfloat GenJet_eta, vfloat GenJet_phi,
                                  vfloat GenJet_mass) {
  vector<TLorentzVector> lvec_jet;
  for (unsigned int i = 0; i < nJet; i++) {
    lvec_jet.push_back(make_lv(Jet_pt[i], Jet_eta[i], Jet_phi[i], Jet_mass[i]));
  }

  vector<TLorentzVector> lvec_genjet;
  for (unsigned int i = 0; i < nGenJet; i++) {
    lvec_genjet.push_back(
        make_lv(GenJet_pt[i], GenJet_eta[i], GenJet_phi[i], GenJet_mass[i]));
  }

  vector<pair<unsigned int, unsigned int>> gen_reco_matches;

  vector<GenJetRecoJetPair> results;

  for (unsigned int i = 0; i < lvec_genjet.size(); i++) {
    unsigned int best_match_reco = 0;
    bool found_match = false;
    double min_dr = 99.0;

    for (unsigned int j = 0; j < lvec_jet.size(); j++) {
      const auto dr = lvec_genjet[i].DeltaR(lvec_jet[j]);
      if ((dr < 0.3) && (dr < min_dr)) {
        best_match_reco = j;
        min_dr = dr;
        found_match = true;
      }
    }

    GenJetRecoJetPair p;
    p.GenJet_pt = lvec_genjet[i].Pt();
    p.GenJet_eta = lvec_genjet[i].Eta();

    if (found_match) {
      gen_reco_matches.push_back(make_pair(i, best_match_reco));
      p.RecoJet_pt = lvec_jet[best_match_reco].Pt();
    }
    results.push_back(p);
  }
  return results;
}

void PrintPairs(vector<GenJetRecoJetPair>& pairs, GenJetRecoJetPair& buf,
                TTree& out_tree) {
  for (auto p : pairs) {
    buf.GenJet_pt = p.GenJet_pt;
    out_tree.Fill();
  }
}

int main(int argc, char* argv[]) {
  TFile* file = TFile::Open(
      "file://./data/ggh_hmumu/0C2B3A66-B042-E811-8C6D-44A8423DE2C0.root");
  ROOT::RDataFrame d(
      "Events", file,
      {
          "nJet", "Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass", "nGenJet",
          "GenJet_pt", "GenJet_eta", "GenJet_phi", "GenJet_mass",
      });

  TFile* outfile = new TFile("out.root", "RECREATE");
  outfile->cd();
  unique_ptr<TTree> out_tree =
      make_unique<TTree>("GenJetRecoJetPair", "Gen - reco jet pair");
  GenJetRecoJetPair buf;
  (*out_tree).Branch("GenJet_pt", &buf.GenJet_pt, "GenJet_pt/F");

  cout << "Running RDataframe reduction..." << endl;
  d.Filter(IsGoodEvent)
      .Define("pairs", DoStuff)
      .Foreach(
          [&buf, &out_tree](vector<GenJetRecoJetPair> p) {
            PrintPairs(p, buf, *out_tree);
          },
          {"pairs"});
  out_tree->Write();
  outfile->Write();
  outfile->Close();

  return 0;
}
