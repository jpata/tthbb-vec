#ifndef NANOFLOW_H
#define NANOFLOW_H

#include <chrono>
#include <iomanip>

#include <TFile.h>
#include <TH1D.h>
#include <TStopwatch.h>
#include <TLorentzVector.h>

#include "json.hpp"

using namespace std;
using nlohmann::json;

#include <TLeaf.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>
#include <ROOT/RDataFrame.hxx>

#include <typeinfo>

using namespace std;

namespace nanoflow {

///////////////////////////////////////////////////////////////////////////////
//                           ╔⏤⏤⏤⏤╝❀╚⏤⏤⏤⏤╗
//                             ANALYSIS CONFIG
//                           ╚⏤⏤⏤⏤╗❀╔⏤⏤⏤⏤╝
///////////////////////////////////////////////////////////////////////////////


// This data structure contains the configuration of the event loop.
// We can load the configuration from a json file
class Configuration {
 public:
  vector<string> input_files;
  string output_filename;
  int max_events;

  //Populate the Configuration from json
  Configuration(const string& json_file) {
    ifstream inp(json_file);
    json input_json;
    inp >> input_json;

    for (auto fn : input_json.at("input_filenames")) {
      input_files.push_back(fn);
    }
    output_filename = input_json.at("output_filename").get<string>();
    max_events = input_json.at("max_events").get<int>();
  }
};

///////////////////////////////////////////////////////////////////////////////
//                           ╔⏤⏤⏤⏤╝❀╚⏤⏤⏤⏤╗
//                               DATA ACCESS
//                           ╚⏤⏤⏤⏤╗❀╔⏤⏤⏤⏤╝
///////////////////////////////////////////////////////////////////////////////

// Compile-time hash function for strings from
// https://github.com/rioki/rex/blob/master/strex.h#L95  By defining this, we
// can access members from the map by a string that is known at compile time
// without having to do runtime string hashing, meaning the code can be fast.
constexpr inline unsigned int string_hash(const char* str, int h = 0) {
  return !str[h] ? 5381 : (string_hash(str, h + 1) * 33) ^ str[h];
}

inline unsigned int string_hash_cpp(const string& str) {
  return string_hash(str.c_str());
}

// Wraps arrays of a specific type from a TTree to TTreeReaderArray-s
template <typename T>
class LazyArrayReader {
 public:
  TTreeReader& reader;

  unordered_map<unsigned int, unique_ptr<TTreeReaderArray<T>>> reader_cache;
  // memory-contiguous data from the arrays
  unordered_map<unsigned int, ROOT::VecOps::RVec<T>> value_cache;

  LazyArrayReader(TTreeReader& _reader) : reader(_reader) {}

  // Creates the TTreeReaderArray for a specific branch on the heap and stores
  // it in the cache
  void setup(const string& id) {
    // const char* tn =
    //     static_cast<TLeaf*>(
    //         reader.GetTree()->GetBranch(id.c_str())->GetListOfLeaves()->At(0))
    //         ->GetTypeName();
    // cout << "Branch: vector " << tn << " " << id << endl;
    const auto id_hash = string_hash_cpp(id);
    if (reader_cache.find(id_hash) == reader_cache.end()) {
      reader_cache[id_hash] =
          make_unique<TTreeReaderArray<T>>(reader, id.c_str());
    }
  }

  void read(const unsigned int& id_hash) {
    value_cache[id_hash] = ROOT::VecOps::RVec<T>(
        (*reader_cache.at(id_hash)).begin(), (*reader_cache.at(id_hash)).end());
  }

  // Gets the value stored in a specific array at a specific index
  inline T get(const unsigned int& id_hash, unsigned int idx) const {
    // const TTreeReaderArray<T>& val = *reader_cache.at(id_hash);
    // return (*reader_cache.at(id_hash))[idx];
    return value_cache.at(id_hash)[idx];
  }

  // Gets the full vector
  inline ROOT::VecOps::RVec<T> get_vec(const unsigned int& id_hash) const {
    return value_cache.at(id_hash);
  }
};

// Wraps numbers (values) of a specific type from a TTree to TTreeReaderValue-s
template <typename T>
class LazyValueReader {
 public:
  unordered_map<unsigned int, unique_ptr<TTreeReaderValue<T>>> reader_cache;
  unordered_map<unsigned int, T> value_cache;
  TTreeReader& reader;

  LazyValueReader(TTreeReader& _reader) : reader(_reader) {}

  void setup(const string& id) {
    // const char* tn =
    //     static_cast<TLeaf*>(
    //         reader.GetTree()->GetBranch(id.c_str())->GetListOfLeaves()->At(0))
    //         ->GetTypeName();
    // cout << "Branch: " << tn << " " << id << endl;
    const auto id_hash = string_hash_cpp(id);
    if (reader_cache.find(id_hash) == reader_cache.end()) {
      reader_cache[id_hash] =
          make_unique<TTreeReaderValue<T>>(reader, id.c_str());
    }
  }

  void read(const unsigned int& id_hash) {
    value_cache[id_hash] = **reader_cache.at(id_hash);
  }

  inline T get(const unsigned int& id_hash) const {
    return value_cache.at(id_hash);
  }
};

// Wraps the full NanoAOD event with branches of different types
// to Array and Value readers automatically
class NanoEvent {
 public:
  LazyArrayReader<Float_t> lc_vfloat;
  LazyArrayReader<Int_t> lc_vint;
  LazyArrayReader<UInt_t> lc_vuint;
  LazyArrayReader<Bool_t> lc_vbool;
  LazyArrayReader<UChar_t> lc_vuchar;

  LazyValueReader<Float_t> lc_float;
  LazyValueReader<Int_t> lc_int;
  LazyValueReader<UInt_t> lc_uint;
  LazyValueReader<Bool_t> lc_bool;
  LazyValueReader<UChar_t> lc_uchar;
  LazyValueReader<ULong64_t> lc_ulong64;

  unsigned int run;
  unsigned int luminosityBlock;
  unsigned long long event;

  // Connects all the existing branches from the TTree to Array and Value
  // readers Unless the readers are accessed, there is no overhead from this.
  NanoEvent(TTreeReader& reader)
      : lc_vfloat(reader),
        lc_vint(reader),
        lc_vuint(reader),
        lc_vbool(reader),
        lc_vuchar(reader),
        lc_float(reader),
        lc_int(reader),
        lc_uint(reader),
        lc_bool(reader),
        lc_uchar(reader),
        lc_ulong64(reader) {
    for (auto leaf_obj : *reader.GetTree()->GetListOfLeaves()) {
      TLeaf* leaf = (TLeaf*)leaf_obj;
      const string dtype(leaf->GetTypeName());
      const string leaf_name(leaf->GetName());

      // cout << leaf_name << " " << dtype << endl;

      if (leaf->GetLeafCount() != nullptr) {
        if (dtype == "Float_t") {
          lc_vfloat.setup(leaf_name);
        } else if (dtype == "Int_t") {
          lc_vint.setup(leaf_name);
        } else if (dtype == "UInt_t") {
          lc_vuint.setup(leaf_name);
        } else if (dtype == "Bool_t") {
          lc_vbool.setup(leaf_name);
        } else if (dtype == "UChar_t") {
          lc_vuchar.setup(leaf_name);
        } else {
          cerr << "Could not understand array dtype " << dtype << " "
               << leaf_name << endl;
        }
      } else if (leaf->GetLeafCount() == nullptr) {
        if (dtype == "Float_t") {
          lc_float.setup(leaf_name);
        } else if (dtype == "Int_t") {
          lc_int.setup(leaf_name);
        } else if (dtype == "UInt_t") {
          lc_uint.setup(leaf_name);
        } else if (dtype == "Bool_t") {
          lc_bool.setup(leaf_name);
        } else if (dtype == "UChar_t") {
          lc_uchar.setup(leaf_name);
        } else if (dtype == "ULong64_t") {
          lc_ulong64.setup(leaf_name);
        } else {
          cerr << "Could not understand number dtype " << dtype << " "
               << leaf_name << endl;
        }
      }
    }
  }  // constructor

  virtual void analyze() = 0;
};

// Accesses data from the underlying TTree wrapped by a NanoEvent lazily
// using get_DTYPE(key) methods, e.g. get_float(string_hash("Jet_pt"))
// Each LazyObject contains an index, which corresponds to the index in the
// NanoAOD arrays.
class LazyObject {
 public:
  // the reference to the parent NanoAOD event
  NanoEvent* event;

  // the index of the object in the object array
  unsigned int index;

  // unordered_map<unsigned int, float> userfloats;
  // mutable unordered_map<unsigned int, float> float_cache;

  // Initializes the object based on the event and the index
  LazyObject(NanoEvent* _event, unsigned int _index)
      : event(_event), index(_index) {}
  virtual ~LazyObject() {};

  // Caching does not seem to be necessary
  // //Retrieves a float from the object
  // Float_t get_float(const unsigned int string_hash) const {
  //     const auto& cache_key = float_cache.find(string_hash);
  //     if (cache_key == float_cache.end()) {
  //         const auto& v = event.lc_vfloat.get(string_hash, index);
  //         float_cache[string_hash] = v;
  //         return v;
  //     }
  //     return cache_key->second;
  // }

  // Retrieves a float from the object
  Float_t get_float(const unsigned int string_hash) const {
    return event->lc_vfloat.get(string_hash, index);
  }

  // Retrieves an int from the object
  Int_t get_int(const unsigned int string_hash) const {
    return event->lc_vint.get(string_hash, index);
  }

};

// This is here to verify the string hashing at compile time
static_assert(string_hash("Jet_pt") == 1724548869,
              "compile-time string hashing failed");


// A class that creates the output file and contains all the other
// output objects: histograms, TTrees etc
class Output {
 public:
  unique_ptr<TFile> outfile;

  // A map of "histo_nickname" -> TH1D
  // However, for reasons of speed, we use a compile-time hash of the string
  // therefore, we only ever refer to the histogram by its hash, which is a
  // number
  unordered_map<unsigned int, shared_ptr<TH1D>> histograms_1d;
  unordered_map<unsigned int, shared_ptr<TTree>> trees;

  // Creates the output TFile
  Output(const string& outfn) {
    cout << "Creating output file " << outfn.c_str() << endl;
    outfile = make_unique<TFile>(outfn.c_str(), "RECREATE");
    outfile->cd();
  }

  // makes sure the TFile is properly written and closed
  void close() {
    cout << "Writing output to file " << outfile->GetPath() << endl;
    outfile->Write();
    outfile->Close();
  }
};

///////////////////////////////////////////////////////////////////////////////
//                           ╔⏤⏤⏤⏤╝❀╚⏤⏤⏤⏤╗
//                                ANALYZERS
//                           ╚⏤⏤⏤⏤╗❀╔⏤⏤⏤⏤╝
///////////////////////////////////////////////////////////////////////////////

// This is a simple template of an Analyzer
// An Analyzer receives a read-only event and processes it in some simple way
class Analyzer {
 public:
  virtual void analyze(NanoEvent& event) = 0;
  virtual const string getName() const = 0;
};

// This is an example of how to produce TTree outputs
class TreeAnalyzer : public Analyzer {
 public:
  Output& output;

  shared_ptr<TTree> out_tree;

  //Variables to keep track of the (run, lumi, event triplet)
  unsigned int br_run;
  unsigned int br_luminosityBlock;
  unsigned long br_event;

  TreeAnalyzer(Output& _output) : output(_output) {
    output.outfile->cd();
    output.trees[string_hash("Events")] =
        make_shared<TTree>("Events", "Events");
    out_tree = output.trees.at(string_hash("Events"));

    out_tree->Branch("run", &br_run, "run/i");
    out_tree->Branch("luminosityBlock", &br_luminosityBlock, "luminosityBlock/i");
    out_tree->Branch("event", &br_event, "event/l");
    // out_tree.lock()->SetDirectory(output->outfile.get());
    // cout << out_tree.lock()->GetName() << endl;
  }

  //Processes the event, possible to override by child classes
  virtual void analyze(NanoEvent& event) {
    br_run = event.run;
    br_luminosityBlock = event.luminosityBlock;
    br_event = event.event;

    out_tree->Fill();
  }

  virtual const string getName() const { return "TreeAnalyzer"; }
};

class FileReport {
 public:
  // Keeps track of the total duration (in nanoseconds) spent on constructing
  // the event representation from the ROOT file
  unsigned long long event_duration;

  // total number of events processed
  unsigned long long num_events_processed;

  // Event loop timing information
  double cpu_time;
  double real_time;
  double speed;

  // input filename
  string filename;

  // Keeps track of the total duration (in nanoseconds) spent on each analyzer
  vector<unsigned long long> analyzer_durations;

  vector<string> analyzer_names;

    FileReport(const string& _filename,
                         const vector<Analyzer*>& analyzers)
      : event_duration(0),
        num_events_processed(0),
        cpu_time(0),
        real_time(0),
        speed(0),
        filename(_filename) {
    // Initialize the analyzer time counters
    for (unsigned int iAnalyzer = 0; iAnalyzer < analyzers.size(); iAnalyzer++) {
      analyzer_durations.push_back(0);
      analyzer_names.push_back(analyzers.at(iAnalyzer)->getName());
    }
  }

  void print(ostream& stream) {
    auto cpu_eff = this->cpu_time / this->real_time;
    vector<double> analyzer_runtime_fracs;
    auto tot_duration =
        accumulate(analyzer_durations.begin(), analyzer_durations.end(), 0.0) +
        event_duration;

    for (auto dur : analyzer_durations) {
      analyzer_runtime_fracs.push_back(dur / tot_duration);
    }
    analyzer_runtime_fracs.push_back(event_duration / tot_duration);

    cpu_eff = this->cpu_time / this->real_time;
    stream << "FileReport eff=" << cpu_eff << ",";

    stream << "NanoEvent=" << analyzer_runtime_fracs[analyzer_names.size()]
           << ",";
    for (unsigned int i = 0; i < analyzer_names.size(); i++) {
      stream << analyzer_names[i] << "=" << analyzer_runtime_fracs[i] << ",";
    }
    stream << endl;
  }

};

// Convert the report from processing one file to json
static inline void to_json(json& j, const FileReport& p) {
  j = json{{"filename", p.filename},
           {"num_events_processed", p.num_events_processed},
           {"cpu_time", p.cpu_time},
           {"real_time", p.real_time},
           {"speed", p.speed},
           {"event_duration", p.event_duration},
           {"analyzer_durations", p.analyzer_durations},
           {"analyzer_names", p.analyzer_names}};
}

///////////////////////////////////////////////////////////////////////////////
//                           ╔⏤⏤⏤⏤╝❀╚⏤⏤⏤⏤╗
//                            UTILITY FUNCTIONS
//                           ╚⏤⏤⏤⏤╗❀╔⏤⏤⏤⏤╝
///////////////////////////////////////////////////////////////////////////////


// Prints the system time as HH::mm::ss
static inline string get_time() {
  auto now = chrono::system_clock::now();
  auto in_time_t = chrono::system_clock::to_time_t(now);

  stringstream ss;
  ss << put_time(localtime(&in_time_t), "%X");
  return ss.str();
}


// Helper function to create a TLorentzVector from spherical coordinates
static inline TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
  TLorentzVector lv;
  lv.SetPtEtaPhiM(pt, eta, phi, mass);
  return lv;
}

///////////////////////////////////////////////////////////////////////////////
//                           ╔⏤⏤⏤⏤╝❀╚⏤⏤⏤⏤╗
//                                MAIN LOOP
//                           ╚⏤⏤⏤⏤╗❀╔⏤⏤⏤⏤╝
///////////////////////////////////////////////////////////////////////////////


// This is the main event loop
// Given a TTreeReader reader, we process all the specified analyzers and store
// the  output in the Output data structure.  You shouldn't have to add anything
// to the event loop if you want to compute a new  quantity - rather, you can
// add a new Analyzer
template <class EventClass, class ConfigurationClass>
FileReport looper_main(const ConfigurationClass& config, const string& filename,
                       TTreeReader& reader, Output& output,
                       const vector<Analyzer*>& analyzers, long long max_events,
                       long long reportevery) {
  // Make sure we clear the state of the reader
  reader.Restart();

  TStopwatch sw;
  sw.Start(); 

  // We initialize the C++ representation of the event (data row) from the
  // TTreeReader
  EventClass event(reader, config);

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

    auto time_t0 = chrono::high_resolution_clock::now();

    // We initialize the event
    event.analyze();

    auto time_t1 = chrono::high_resolution_clock::now();
    auto time_dt =
        chrono::duration_cast<chrono::nanoseconds>(time_t1 - time_t0)
            .count();
    report.event_duration += time_dt;

    unsigned int iAnalyzer = 0;

    // We run all the analyzers one after the other
    for (auto* analyzer : analyzers) {
      auto time_t0 = chrono::high_resolution_clock::now();

      // Here we do the actual work for the analyzer
      analyzer->analyze(event);

      // Get the time in nanoseconds spent per event for this analyzer
      auto time_t1 = chrono::high_resolution_clock::now();
      auto time_dt = chrono::duration_cast<chrono::nanoseconds>(
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
} //looper_main

} // namespace nanoflow
#endif