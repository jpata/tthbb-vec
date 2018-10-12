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

//Event class
void Event::clear_event() {
    jets.clear();
    muons.clear();
    electrons.clear();
    highest_inv_mass = 0;
}

void Event::analyze() {
    clear_event();

    //Get the number of jets as an uint
    const auto nJet = this->lc_uint.get(string_hash("nJet"));
    //Construct the jet objects from the branches
    for (unsigned int _nJet = 0; _nJet < nJet; _nJet++) {
        Jet jet(*this, _nJet);
        jets.push_back(jet);
    }

    //Construct muons
    const auto nMuon = this->lc_uint.get(string_hash("nMuon"));
    for (unsigned int _nMuon = 0; _nMuon < nMuon; _nMuon++) {
        Muon muon(*this, _nMuon);
        muons.push_back(muon);
    }

    //Construct electrons
    const auto nElectron = this->lc_uint.get(string_hash("nElectron"));
    for (unsigned int _nElectron = 0; _nElectron < nElectron; _nElectron++) {
        Electron electron(*this, _nElectron);
        electrons.push_back(electron);
    }
}

FileReport looper_main(
    const string& filename,
    TTreeReader& reader,
    Output& output,
    const vector<Analyzer*>& analyzers,
    long long max_events
    ) {

    //Make sure we clear the state of the reader
    reader.Restart();
    
    TStopwatch sw;
    sw.Start();

    //We initialize the event from the TTreeReader
    Event event(reader);

    //Keep track of the number of events we processed
    unsigned long long nevents = 0;

    //Keep track of the total time per event
    FileReport report(filename, analyzers);

    //This is the main event loop
    cout << "starting loop over " << reader.GetEntries(true) << " events in TTree " << reader.GetTree() << endl;
    while (reader.Next()) {

        //In case of early termination
        if (max_events > 0 && nevents == max_events) {
            cout << "breaking event loop before event " << nevents << endl;
            break;
        }

        auto time_t0 = std::chrono::high_resolution_clock::now();

        //We create the event from the NanoAOD data
        //This reads some data from the ROOT file, but not the whole file
        event.analyze();

        auto time_t1 = std::chrono::high_resolution_clock::now();
        auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0).count();
        report.event_duration += time_dt;

        unsigned int iAnalyzer = 0;

        //We run all the analyzers one after the other
        for (auto* analyzer : analyzers) {

            auto time_t0 = std::chrono::high_resolution_clock::now();

            //Here we do the actual work for the analyzer
            analyzer->analyze(event);

            //Get the time in nanoseconds spent per event for this analyzer
            auto time_t1 = std::chrono::high_resolution_clock::now();
            auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0).count();
            report.analyzer_durations[iAnalyzer] += time_dt;

            iAnalyzer += 1;
        }

        //Print out a progress report
        if (nevents % 50000 == 0) {
            const auto elapsed_time = sw.RealTime();
            cout << "Processed " << nevents << "/" << reader.GetEntries(true) << " speed=" << nevents/elapsed_time/1000.0 << "kHz" << endl;
            sw.Continue();
        }
        nevents += 1;
    }
    report.num_events_processed = nevents;

    sw.Stop();

    //Print out some statistics
    report.cpu_time = sw.CpuTime();
    report.real_time = sw.RealTime();

    //Compute the event processing speed in kHz
    report.speed = (double)report.num_events_processed / report.real_time / 1000.0;
    
    report.print(cout);

    cout << "looper_main" << " nevents=" << report.num_events_processed << ",cpu_time=" << report.cpu_time << ",real_time=" << report.real_time << ",speed=" << report.speed << endl;

    return report;
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

void TreeAnalyzer::analyze(Event& event) {
    br_run = event.lc_uint.get(string_hash("run"));
    br_luminosityBlock = event.lc_uint.get(string_hash("luminosityBlock"));
    br_event = event.lc_ulong64.get(string_hash("event"));

    out_tree->Fill();
}

const string TreeAnalyzer::getName() const {
    return "TreeAnalyzer";
}
