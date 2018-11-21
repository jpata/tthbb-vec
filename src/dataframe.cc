#include <iostream>
#include <map>
#include <string>

#include <TFile.h>
#include <TROOT.h>
#include <ROOT/RDataFrame.hxx>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "df /path/to/nano/input.root /path/to/output.root" << endl;
    return 0;
  }
  const char* input_path = argv[1];
  const char* output_path = argv[2];

  // ROOT::EnableImplicitMT();
  TFile* file = TFile::Open(input_path);
  const vector<string> cols = {
      "nGenMediator",       "GenInitialState_pz", "lep2_highest_inv_mass",
      "nGenFinalStateMuon", "me_gen_sig",         "me_gen_bkg",
      "me_reco_sig",        "me_reco_bkg",        "nMuon",
      "nMuon_match"};
  const vector<string> cols_save = {
      "nGenMediator",       "lep2_highest_inv_mass",
      "nGenFinalStateMuon", "me_gen_sig",
      "me_gen_bkg",         "me_reco_sig",
      "me_reco_bkg",        "nMuon",
      "nMuon_match",        "GenInitialState_pz_0",
      "Muon_px_0",          "Muon_py_0",
      "Muon_pz_0",          "Muon_energy_0",
      "Muon_px_1",          "Muon_py_1",
      "Muon_pz_1",          "Muon_energy_1"};
  const vector<string> cols_full = {
      "GenInitialState_pz_0", "GenInitialState_pdgId_0",
      "GenInitialState_pdgId_1", "lep2_highest_inv_mass", "reco_fs_pz"};

  ROOT::RDataFrame df("Events", file, cols);

  auto df_extended =
      df.Filter("nMuon==2 && abs(lep2_highest_inv_mass - 125) < 30")
          .Define("GenInitialState_pz_0",
                  [](ROOT::VecOps::RVec<float> GenInitialState_pz) {
                    return GenInitialState_pz[0];
                  },
                  {"GenInitialState_pz"})
          .Define("GenInitialState_pdgId_0",
                  [](ROOT::VecOps::RVec<int> GenInitialState_pdgId) {
                    return GenInitialState_pdgId[0];
                  },
                  {"GenInitialState_pdgId"})
          .Define("GenInitialState_pdgId_1",
                  [](ROOT::VecOps::RVec<int> GenInitialState_pdgId) {
                    return GenInitialState_pdgId[1];
                  },
                  {"GenInitialState_pdgId"});

  df_extended.Snapshot("Events_all", output_path, cols_full);

  ROOT::RDF::RSnapshotOptions opts;
  opts.fMode = "UPDATE";
  auto df_flattened =
      df_extended
          .Define("Muon_px_0",
                  [](ROOT::VecOps::RVec<float> Muon_px) { return Muon_px[0]; },
                  {"Muon_px"})
          .Define("Muon_px_1",
                  [](ROOT::VecOps::RVec<float> Muon_px) { return Muon_px[1]; },
                  {"Muon_px"})
          .Define("Muon_py_0",
                  [](ROOT::VecOps::RVec<float> Muon_py) { return Muon_py[0]; },
                  {"Muon_py"})
          .Define("Muon_py_1",
                  [](ROOT::VecOps::RVec<float> Muon_py) { return Muon_py[1]; },
                  {"Muon_py"})
          .Define("Muon_pz_0",
                  [](ROOT::VecOps::RVec<float> Muon_pz) { return Muon_pz[0]; },
                  {"Muon_pz"})
          .Define("Muon_pz_1",
                  [](ROOT::VecOps::RVec<float> Muon_pz) { return Muon_pz[1]; },
                  {"Muon_pz"})
          .Define("Muon_energy_0",
                  [](ROOT::VecOps::RVec<float> Muon_energy) {
                    return Muon_energy[0];
                  },
                  {"Muon_energy"})
          .Define("Muon_energy_1",
                  [](ROOT::VecOps::RVec<float> Muon_energy) {
                    return Muon_energy[1];
                  },
                  {"Muon_energy"});
  df_flattened.Snapshot("Events", output_path, cols_save, opts);

  return 0;
}
