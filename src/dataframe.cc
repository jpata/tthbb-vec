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

  ROOT::EnableImplicitMT();
  TFile* file = TFile::Open(input_path);
  const vector<string> cols = {
     "nGenMediator", "lep2_highest_inv_mass", "nGenFinalStateMuon", "me_gen_sig", "me_gen_bkg", "me_reco_sig", "me_reco_bkg", "nMuon", "nMuon_match"
  };
  ROOT::RDataFrame df("Events", file, cols);

  df.Filter("nGenMediator==1 && abs(lep2_highest_inv_mass - 125)<5").Snapshot("Events", output_path, cols);
  return 0;
}
