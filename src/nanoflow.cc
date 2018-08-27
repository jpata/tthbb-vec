#include "nanoflow.h"

//Output class
Output::Output(const std::string& outfn) {
    outfile = std::move(std::unique_ptr<TFile>(new TFile(outfn.c_str(), "RECREATE")));
    outfile->cd();
}

Output::~Output() {
    outfile->Write();
    outfile->Close();
}

//Event class
void Event::clear_event() {
    jets.clear();
    muons.clear();
    electrons.clear();
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

    //Construct euons
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
    const std::vector<std::shared_ptr<Analyzer>>& analyzers) {

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
    while (reader.Next()) {

        auto time_t0 = std::chrono::high_resolution_clock::now();

        //We create the event from the NanoAOD data
        //This reads some data from the ROOT file, but not the whole file
        event.analyze();

        auto time_t1 = std::chrono::high_resolution_clock::now();
        auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0).count();
        report.event_duration += time_dt;

        unsigned int iAnalyzer = 0;

        //We run all the analyzers one after the other
        for (const auto& analyzer : analyzers) {

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
        if (nevents > 0  && nevents % 50000 == 0) {
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

    cout << "looper_main" << " nevents=" << report.num_events_processed << " cpu_time=" << report.cpu_time << ",real_time=" << report.real_time << ",speed=" << report.speed << " kHz" << endl;

    return report;
}

std::_Put_time<char> get_time() {
    using std::chrono::system_clock;
    std::time_t tt = system_clock::to_time_t (system_clock::now());

    struct std::tm * ptm = std::localtime(&tt);
    return std::put_time(ptm,"%X");
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

FileReport::FileReport(const std::string& _filename, const vector<std::shared_ptr<Analyzer>>& analyzers) :
    event_duration(0),
    num_events_processed(0),
    cpu_time(0),
    real_time(0),
    speed(0),
    filename(_filename) {
    
    //Initialize the analyzer time counters
    for(unsigned int iAnalyzer = 0; iAnalyzer < analyzers.size(); iAnalyzer++) {
        analyzer_durations.push_back(0);
    }
}

void to_json(json& j, const FileReport& p) {
    j = json{
        {"filename", p.filename},
        {"num_events_processed", p.num_events_processed},
        {"cpu_time", p.cpu_time},
        {"real_time", p.real_time},
        {"speed", p.speed},
        {"event_duration", p.event_duration},
        {"analyzer_durations", p.analyzer_durations}
    };
}
