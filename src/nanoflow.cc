#include "nanoflow.h"

//Output class
Output::Output(const std::string& outfn) {
    cout << "Creating output file " << outfn.c_str() << endl;
    //outfile = std::move(std::unique_ptr<TFile>(new TFile(outfn.c_str(), "RECREATE")));
    outfile = make_unique<TFile>(outfn.c_str(), "RECREATE");
    outfile->cd();
}

void Output::close() {
    cout << "Writing output to file " << outfile->GetPath() << endl;
    outfile->Write();
    outfile->Close();
}



std::string get_time()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%X");
    return ss.str();
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

FileReport::FileReport(const std::string& _filename, const vector<Analyzer*>& analyzers) :
    event_duration(0),
    num_events_processed(0),
    cpu_time(0),
    real_time(0),
    speed(0),
    filename(_filename) {
    
    //Initialize the analyzer time counters
    for (unsigned int iAnalyzer = 0; iAnalyzer < analyzers.size(); iAnalyzer++) {
        analyzer_durations.push_back(0);
        analyzer_names.push_back(analyzers.at(iAnalyzer)->getName());
    }
}

void FileReport::print(std::ostream& stream) {
    auto cpu_eff = this->cpu_time / this->real_time;
    vector<double> analyzer_runtime_fracs;
    auto tot_duration = accumulate(analyzer_durations.begin(), analyzer_durations.end(), 0.0);
 
    for (auto dur : analyzer_durations) {
        analyzer_runtime_fracs.push_back(dur / tot_duration);
    }

    cpu_eff = this->cpu_time / this->real_time;
    stream << "FileReport: eff=" << cpu_eff << ",";

    for (unsigned int i = 0; i < analyzer_names.size(); i++) {
        stream << analyzer_names[i] << "=" << analyzer_runtime_fracs[i] << ",";        
    }
    stream << endl;

}

//Convert the report from processing one file to json
void to_json(json& j, const FileReport& p) {
    j = json{
        {"filename", p.filename},
        {"num_events_processed", p.num_events_processed},
        {"cpu_time", p.cpu_time},
        {"real_time", p.real_time},
        {"speed", p.speed},
        {"event_duration", p.event_duration},
        {"analyzer_durations", p.analyzer_durations},
        {"analyzer_names", p.analyzer_names}
    };
}

TreeAnalyzer::TreeAnalyzer(Output& _output) : output(_output) {
    output.outfile->cd();
    output.trees[string_hash("Events")] = std::make_shared<TTree>("Events", "Events");
    out_tree = output.trees.at(string_hash("Events")); 

    out_tree->Branch("run", &br_run, "run/i");
    out_tree->Branch("luminosityBlock", &br_luminosityBlock, "luminosityBlock/i");
    out_tree->Branch("event", &br_event, "event/l");
    //out_tree.lock()->SetDirectory(output->outfile.get());
    //cout << out_tree.lock()->GetName() << endl;
}

void TreeAnalyzer::analyze(NanoEvent& event) {
    br_run = event.lc_uint.get(string_hash("run"));
    br_luminosityBlock = event.lc_uint.get(string_hash("luminosityBlock"));
    br_event = event.lc_ulong64.get(string_hash("event"));

    out_tree->Fill();
}

const string TreeAnalyzer::getName() const {
    return "TreeAnalyzer";
}
