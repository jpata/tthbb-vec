// Compile with:
// c++ `root-config --cflags --libs --evelibs` -Wno-unsequenced -O3
// code/simple_loop.cc -o simple_loop

#include <iostream>
#include <map>
#include <string>

#include <TFile.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TStopwatch.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

using namespace std;

// Do a simple array-based loop on NanoAOD and saves the output to a TFile in
// out.root  More info on NanoAOD is located here:
// https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookNanoAOD
void loop_plain(TTreeReader& reader) {
  reader.Restart();

  TStopwatch sw;

  //const unsigned int nbytes = 0;

  // As usual in C++ ROOT, we have to predefine every branch and this can get
  // annoying, which is why I've developed a simple wrapper  You can find the
  // list of all branches with description here:
  // https://cms-nanoaod-integration.web.cern.ch/integration/master/mc94X_doc.html
  TTreeReaderValue<unsigned int> nJet(reader, "nJet");
  TTreeReaderArray<float> Jet_pt(reader, "Jet_pt");
  TTreeReaderArray<float> Jet_eta(reader, "Jet_eta");
  TTreeReaderArray<float> Jet_phi(reader, "Jet_phi");
  TTreeReaderArray<float> Jet_mass(reader, "Jet_mass");

  unsigned int nevents = 0;
  TFile out("out.root", "RECREATE");
  TH1D h_sumpt("sumpt", "sumpt", 100, 0, 1000);

  sw.Start();

  while (reader.Next()) {
    double sum_pt = 0.0;

    // Simple numbers like nJet can be accessed using *nJet
    for (unsigned int _nJet = 0; _nJet < *nJet; _nJet++) {
      // Arrays can be accessed like this
      const auto pt = Jet_pt[_nJet];
      // const auto eta = Jet_eta[_nJet];
      // const auto phi = Jet_phi[_nJet];
      // const auto mass = Jet_mass[_nJet];
      sum_pt += pt;
    }
    h_sumpt.Fill(sum_pt);
    nevents += 1;
  }

  sw.Stop();
  const auto cpu_time = sw.CpuTime();
  const auto real_time = sw.RealTime();

  const auto speed = (double)nevents / real_time / 1000.0;
  cout << "h_sumpt " << h_sumpt.GetEntries() << " " << h_sumpt.GetMean()
       << endl;

  cout << "loop_plain "
       << "cpu_time=" << cpu_time << ",real_time=" << real_time
       << ",speed=" << speed << " kHz" << endl;

  out.Write();
  out.Close();
}

int main(int argc, char* argv[]) {
  gROOT->SetBatch(true);

  // Check command line arguments
  if (argc != 2) {
    cerr << "simple_loop /path/to/nano/input.root" << endl;
    return 0;
  }

  TFile tf(argv[1]);
  TTreeReader reader("Events", &tf);

  // Run the event loop
  loop_plain(reader);
  return 0;
}
