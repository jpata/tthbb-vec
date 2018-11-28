#include <iostream>
#include <map>
#include <string>

#include <TFile.h>
#include <TROOT.h>

#include "demoanalysis.h"

using namespace nanoflow;

int main(int argc, char* argv[]) {
  using json = nlohmann::json;

  cout << get_time() << " nanoflow main() started" << endl;
  gROOT->SetBatch(true);

  if (argc != 3) {
    cerr << "Usage: ./nf input.json output.json" << endl;
    return 0;
  }

  cout << get_time() << " loading json file " << argv[1] << endl;

  // Load the configuration from the provided input json file
  std::unique_ptr<Configuration> conf =
      std::make_unique<Configuration>(argv[1]);

  // Create the output file
  std::unique_ptr<Output> output =
      std::make_unique<Output>(conf->output_filename);

  // Define the sequence of analyzers you want to run
  // These are defined in the myanalyzers.h/myanalyzers.cc files
  vector<Analyzer*> analyzers = {
      new MuonEventAnalyzer(*output),
      new MyTreeAnalyzer(*output)
  };

  // Define the final output report
  json total_report;

  // Loop over all the input files
  for (const auto& input_file : conf->input_files) {
    TFile* tf = TFile::Open(input_file.c_str());
    if (tf == nullptr) {
      cerr << "Could not open file " << input_file << ", exiting" << endl;
      return 1;
    }

    // Inititalize the input TTree
    TTreeReader reader("Events", tf);

    // call the main loop
    auto report =
        looper_main<MyAnalysisEvent, Configuration>(*conf, input_file, reader, *output, analyzers, conf->max_events, 1000);
    total_report.push_back(report);
    tf->Close();
  }
  output->close();

  cout << get_time() << " nanoflow main() done on json file " << argv[1] << endl;

  // Write the output metadata json
  std::ofstream out_json(argv[2]);
  out_json << total_report.dump(4);

  return 0;
}
