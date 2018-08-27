#include <iostream>
#include <map>
#include <string>

#include <TFile.h>
#include <TROOT.h>

//general framework declarations
#include "nanoflow.h"

//our own custom analyzers
#include "myanalyzers.h"


int main( int argc, char *argv[]) {
    using json = nlohmann::json;

    cout << get_time() << " looper main() started" << endl;
    gROOT->SetBatch(true); 
    
    if(argc!=3) {
        cerr << "Usage: ./looper input.json output.json" << endl;
        return 0;
    }

    //Load the configuration from the provided input json file
    Configuration conf(argv[1]);

    cout << get_time() << " loaded json file " << argv[1] << endl;

    //Create all the outputs (histograms, trees, ...)
    Output output(conf.output_filename);

    //Create all the Analyzers: later this can be done directly from the JSON
    //These are defined in the myanalyzers.h/myanalyzers.cc files
    vector<std::shared_ptr<Analyzer>> analyzers = {
        std::make_shared<SumPtAnalyzer>(output),
        std::make_shared<EventVarsAnalyzer>(output),
        std::make_shared<JetDeltaRAnalyzer>(output)
    };
    
    //Process all the input files
    json total_report;
    //Loop over all the input files
    for (const auto& input_file : conf.input_files) {
        TFile* tf = TFile::Open(input_file.c_str());
        TTreeReader reader("Events", tf);

        auto report = looper_main(input_file, reader, output, analyzers);
        total_report.push_back(report);
        tf->Close();
    }
    cout << get_time() << " looper main() done on json file " << argv[1] << endl;

    //Write the output metadata json
    std::ofstream out_json(argv[2]);
    out_json << total_report.dump(4);

    return 0;
}
