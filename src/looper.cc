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

    cout << get_time() << " loading json file " << argv[1] << endl;
    
    //Load the configuration from the provided input json file
    std::unique_ptr<Configuration> conf = std::make_unique<Configuration>(argv[1]);

    //Create the output file
    std::unique_ptr<Output> output = std::make_unique<Output>(conf->output_filename);

    //Define the sequence of analyzers you want to run
    //These are defined in the myanalyzers.h/myanalyzers.cc files
    vector<Analyzer*> analyzers = {
        new SumPtAnalyzer(*output),
        new EventVarsAnalyzer(*output),
        //std::make_shared<JetDeltaRAnalyzer>(output),
        new TreeAnalyzer(*output)
    };
   
    //Define the final output report 
    json total_report;

    //Loop over all the input files
    for (const auto& input_file : conf->input_files) {
        TFile* tf = TFile::Open(input_file.c_str());
        if (tf == nullptr) {
            cerr << "Could not open file " << input_file << ", exiting" << endl;
            return 1;
        }
        TTreeReader reader("Events", tf);

        auto report = looper_main(input_file, reader, *output, analyzers);
        total_report.push_back(report);
        tf->Close();
    }
    output->close();

    cout << get_time() << " looper main() done on json file " << argv[1] << endl;

    //Write the output metadata json
    std::ofstream out_json(argv[2]);
    out_json << total_report.dump(4);

    return 0;
}
