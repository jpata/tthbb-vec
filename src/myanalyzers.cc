#include "myanalyzers.h"

// We need to write separate constructors for these objects,
// as the branch names will be resolved to branch pointers at compile time

Muon::Muon(NanoEvent* _event, unsigned int _index)
    : LazyObject(_event, _index),
      FourMomentumSpherical(this->get_float(string_hash("Muon_pt")),
                            this->get_float(string_hash("Muon_eta")),
                            this->get_float(string_hash("Muon_phi")),
                            this->get_float(string_hash("Muon_mass"))),
      matchidx(-1) {}


MuonEventAnalyzer::MuonEventAnalyzer(Output& _output) : output(_output) {
  cout << "Creating MuonEventAnalyzer" << endl;
}

void MuonEventAnalyzer::analyze(NanoEvent& _event) {
  auto& event = static_cast<MyAnalysisEvent&>(_event);

  // Read the necessary branches from disk
  event.lc_uint.read(string_hash("nMuon"));
  event.lc_vfloat.read(string_hash("Muon_pt"));
  event.lc_vfloat.read(string_hash("Muon_eta"));
  event.lc_vfloat.read(string_hash("Muon_phi"));
  event.lc_vfloat.read(string_hash("Muon_mass"));

  // Construct muon objects from the branches and put them to the event
  const auto nMuon = event.lc_uint.get(string_hash("nMuon"));
  for (unsigned int _nMuon = 0; _nMuon < nMuon; _nMuon++) {
    Muon muon(&event, _nMuon);
    event.muons.push_back(muon);
  }
}

const string MuonEventAnalyzer::getName() const { return "MuonEventAnalyzer"; }

Configuration::Configuration(const std::string& json_file) {
  ifstream inp(json_file);
  json input_json;
  inp >> input_json;

  for (auto fn : input_json.at("input_filenames")) {
    input_files.push_back(fn);
  }
  output_filename = input_json.at("output_filename").get<std::string>();
  max_events = input_json.at("max_events").get<int>();
}

MyAnalysisEvent::MyAnalysisEvent(TTreeReader& _reader, const Configuration& _config)
    : NanoEvent(_reader), config(_config) {}

// This is very important to make sure that we always start with a clean
// event and we don't keep any information from previous events
void MyAnalysisEvent::clear_event() {
  muons.clear();

  nMuon = 0;
}

// In this function we create our event representation
// In order to have a fast runtime, we need to do the
// absolute minimum here.
void MyAnalysisEvent::analyze() {
  clear_event();

  this->lc_uint.read(string_hash("run"));
  this->lc_uint.read(string_hash("luminosityBlock"));
  this->lc_ulong64.read(string_hash("event"));

  this->run = this->lc_uint.get(string_hash("run"));
  this->luminosityBlock = this->lc_uint.get(string_hash("luminosityBlock"));
  this->event = this->lc_ulong64.get(string_hash("event"));
}

MyTreeAnalyzer::MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {

  out_tree->Branch("nMuon", &nMuon, "nMuon/I");
  out_tree->Branch("Muon_px", Muon_px.data(), "Muon_px[nMuon]/F");
  out_tree->Branch("Muon_py", Muon_py.data(), "Muon_py[nMuon]/F");
  out_tree->Branch("Muon_pz", Muon_pz.data(), "Muon_pz[nMuon]/F");
  out_tree->Branch("Muon_energy", Muon_energy.data(), "Muon_energy[nMuon]/F");
  out_tree->Branch("Muon_matchidx", Muon_matchidx.data(),
                   "Muon_matchidx[nMuon]/I");
}

void MyTreeAnalyzer::clear() {
  nMuon = 0;

  nMuon = 0;
  Muon_px.fill(0.0);
  Muon_py.fill(0.0);
  Muon_pz.fill(0.0);
  Muon_energy.fill(0.0);
  Muon_matchidx.fill(0);
}

void MyTreeAnalyzer::fill_muon(MyAnalysisEvent& event, vector<Muon>& src) {
  if (src.size() > Muon_px.size()) {
    cerr << "event " << event.event << " " << src.size() << endl;
  }
  nMuon = static_cast<int>(src.size());

  unsigned int i = 0;
  for (auto& gp : src) {
    if (i >= Muon_px.size()) {
      cerr << "ERROR: Muon out of range" << endl;
      break;
    }
    const auto lv = make_lv(gp.pt(), gp.eta(), gp.phi(), gp.mass());
    Muon_px[i] = static_cast<float>(lv.Px());
    Muon_py[i] = static_cast<float>(lv.Py());
    Muon_pz[i] = static_cast<float>(lv.Pz());
    Muon_energy[i] = static_cast<float>(lv.Energy());
    Muon_matchidx[i] = src.at(i).matchidx;
    i += 1;
  }
}

void MyTreeAnalyzer::analyze(NanoEvent& _event) {
  this->clear();

  auto& event = static_cast<MyAnalysisEvent&>(_event);
  nMuon = event.nMuon;

  fill_muon(event, event.muons);

  TreeAnalyzer::analyze(event);
}

const string MyTreeAnalyzer::getName() const { return "MyTreeAnalyzer"; }

FileReport looper_main(const Configuration& config, const string& filename,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers, long long max_events,
                       long long reportevery) {
  // Make sure we clear the state of the reader
  reader.Restart();

  TStopwatch sw;
  sw.Start();

  // We initialize the C++ representation of the event (data row) from the
  // TTreeReader
  MyAnalysisEvent event(reader, config);

  // Keep track of the number of events we processed
  unsigned long long nevents = 0;

  // Keep track of the total time per event
  FileReport report(filename, analyzers);

  // Start the loop over the TTree events
  cout << "starting loop over " << reader.GetEntries(true)
       << " events in TTree " << reader.GetTree() << endl;
  while (reader.Next()) {
    // In case of early termination
    if (max_events > 0 && nevents == max_events) {
      cout << "breaking event loop before event " << nevents << endl;
      break;
    }

    auto time_t0 = std::chrono::high_resolution_clock::now();

    // We initialize the event
    event.analyze();

    auto time_t1 = std::chrono::high_resolution_clock::now();
    auto time_dt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0)
            .count();
    report.event_duration += time_dt;

    unsigned int iAnalyzer = 0;

    // We run all the analyzers one after the other
    for (auto* analyzer : analyzers) {
      auto time_t0 = std::chrono::high_resolution_clock::now();

      // Here we do the actual work for the analyzer
      analyzer->analyze(event);

      // Get the time in nanoseconds spent per event for this analyzer
      auto time_t1 = std::chrono::high_resolution_clock::now();
      auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         time_t1 - time_t0)
                         .count();
      report.analyzer_durations[iAnalyzer] += time_dt;

      iAnalyzer += 1;
    }

    // Print out a progress report
    if (nevents % reportevery == 0) {
      const auto elapsed_time = sw.RealTime();
      const auto speed = nevents / elapsed_time;
      const auto remaining_events = (reader.GetEntries(true) - nevents);
      const auto remaining_time = remaining_events / speed;

      cout << "Processed " << nevents << "/" << reader.GetEntries(true)
           << " speed=" << speed / 1000.0 << "kHz ETA=" << remaining_time << "s"
           << endl;
      sw.Continue();
    }
    nevents += 1;
  }
  report.num_events_processed = nevents;

  sw.Stop();

  // Print out some statistics
  report.cpu_time = sw.CpuTime();
  report.real_time = sw.RealTime();

  // Compute the event processing speed in kHz
  report.speed =
      (double)report.num_events_processed / report.real_time / 1000.0;

  report.print(cout);

  cout << "looper_main"
       << " nevents=" << report.num_events_processed
       << ",cpu_time=" << report.cpu_time << ",real_time=" << report.real_time
       << ",speed=" << report.speed << endl;

  return report;
}
