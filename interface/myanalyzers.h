//In this file, we declare our own custom analyzers, based on the interface defined in nanoflow.h
#ifndef MYANALYZERS_H
#define MYANALYZERS_H

#include <TLorentzVector.h>

#include "nanoflow.h"

TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
    TLorentzVector lv;
    lv.SetPtEtaPhiM(pt, eta, phi, mass);
    return lv;
}

//We can specialize the LazyObject for specific physics objects
//by wrapping the most commonly used branches.
//Jet type based on the on-demand reading of quantities from the underlying TTree
class Jet : public LazyObject {
public:
    const float _pt, _eta, _phi, _mass;
    Jet(const NanoEvent& _event, unsigned int _index) :
    LazyObject(_event, _index),
    _pt(this->get_float(string_hash("Jet_pt"))),
    _eta(this->get_float(string_hash("Jet_eta"))),
    _phi(this->get_float(string_hash("Jet_phi"))),
    _mass(this->get_float(string_hash("Jet_mass"))) {
    }

    float pt() const {
        return _pt;
    }

    float eta() const {
        return _eta;
    }

    float phi() const {
        return _phi;
    }

    float mass() const  {
        return _mass;
    }
};

class Muon : public LazyObject {
public:
    Muon(const NanoEvent& _event, unsigned int _index) : LazyObject(_event, _index) {
    }

    float pt() const {
        return this->get_float(string_hash("Muon_pt"));
    }

    float eta() const {
        return this->get_float(string_hash("Muon_eta"));
    }

    float phi() const {
        return this->get_float(string_hash("Muon_phi"));
    }

    float mass() const  {
        return this->get_float(string_hash("Muon_mass"));
    }
};

class Electron : public LazyObject {
public:
    Electron(const NanoEvent& _event, unsigned int _index) : LazyObject(_event, _index) {
    }

    float pt() const {
        return this->get_float(string_hash("Electron_pt"));
    }

    float eta() const {
        return this->get_float(string_hash("Electron_eta"));
    }

    float phi() const {
        return this->get_float(string_hash("Electron_phi"));
    }

    float mass() const  {
        return this->get_float(string_hash("Electron_mass"));
    }
};

//This data structure contains the configuration of the event loop.
//Currently, this is only the input and output files.
//We can load the configuration from a json file
class Configuration {
public:
    vector<string> input_files;
    string output_filename;
    
    Configuration(const std::string& json_file) {
        ifstream inp(json_file);
        json input_json;
        inp >> input_json;
    
        for (auto fn : input_json.at("input_filenames")) {
          input_files.push_back(fn);
        }
        output_filename = input_json.at("output_filename").get<std::string>();
    }
};

//This is our basic Event representation
//We always construct vectors of basic physics objects such as jets and leptons
//On the other hand, since this is done for every event, we want to keep it as
//minimal as possible.
class Event : public NanoEvent {
public:

    const Configuration& config;

    //We need to predefine the event content here

    //Physics objects
    vector<Jet> jets;
    vector<Muon> muons;
    vector<Electron> electrons;

    //Simple variables
    double lep2_highest_inv_mass;

    Event(TTreeReader& _reader, const Configuration& _config) : NanoEvent(_reader), config(_config) {}

    //This is very important to make sure that we always start with a clean
    //event and we don't keep any information from previous events
    void clear_event() {
        jets.clear();
        muons.clear();
        electrons.clear();
        lep2_highest_inv_mass = 0;
    }

    //In this function we create our event representation
    //In order to have a fast runtime, we need to do the
    //absolute minimum here.
    void analyze() {
        clear_event();
    }

};

class MuonEventAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    MuonEventAnalyzer(Output& _output) : output(_output) {
        cout << "Creating MuonEventAnalyzer" << endl;
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        
        const auto nMuon = event.lc_uint.get(string_hash("nMuon"));
        for (unsigned int _nMuon = 0; _nMuon < nMuon; _nMuon++) {
            Muon muon(event, _nMuon);
            event.muons.push_back(muon);
        }

    }

    virtual const string getName() const override {
        return "MuonEventAnalyzer";
    }
};

class ElectronEventAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    ElectronEventAnalyzer(Output& _output) : output(_output) {
        cout << "Creating ElectronEventAnalyzer" << endl;
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        
        const auto nElectron = event.lc_uint.get(string_hash("nElectron"));
        for (unsigned int _nElectron = 0; _nElectron < nElectron; _nElectron++) {
            Electron electron(event, _nElectron);
            event.electrons.push_back(electron);
        }

    }

    virtual const string getName() const override {
        return "ElectronEventAnalyzer";
    }
};


class JetEventAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    JetEventAnalyzer(Output& _output) : output(_output) {
        cout << "Creating JetEventAnalyzer" << endl;
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        
        //Get the number of jets as an uint
        const auto nJet = event.lc_uint.get(string_hash("nJet"));
        
        //Construct the jet objects from the branches
        for (unsigned int _nJet = 0; _nJet < nJet; _nJet++) {
            Jet jet(event, _nJet);
            event.jets.push_back(jet);
        }

    }

    virtual const string getName() const override {
        return "JetEventAnalyzer";
    }
};


//This example Analyzer computes the sum(pt) of all the jets, muons and electrons in the event
//and stores it in a histogram. The actual implementation code is located in myanalyzers.cc,
//so we can compile it separately.
class SumPtAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    shared_ptr<TH1D> h_sumpt;
    SumPtAnalyzer(Output& _output) : output(_output) {
        cout << "Creating SumPtAnalyzer" << endl;
        //Create the sumpt histogram in the output data structure
        output.outfile->cd();
        output.histograms_1d[string_hash("h_sumpt")] = make_shared<TH1D>("h_sumpt", "sumpt", 100, 0, 1000);
    
        //Retrieve the pointer to the sumpt histogram for convenient access
        h_sumpt = output.histograms_1d.at(string_hash("h_sumpt"));
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        double sum_pt = 0.0;
    
        for (auto jet : event.jets) {
            sum_pt += jet.pt();
        }
        for (auto muon : event.muons) {
            sum_pt += muon.pt();
        }
        for (auto electron : event.electrons) {
            sum_pt += electron.pt();
        }
        h_sumpt->Fill(sum_pt);
    }

    virtual const string getName() const override {
        return "SumPtAnalyzer";
    }
};


//This example Analyzer simply saves the number of primary vertices in an output histogram
class EventVarsAnalyzer : public Analyzer {
public:
    Output& output;

    //Convenient short-hand access to the PV histogram
    shared_ptr<TH1D> h_nPVs;
    EventVarsAnalyzer(Output& _output) : output(_output) {
        cout << "Creating EventVarsAnalyzer" << endl;
    
        //Create and store the nPV histogram in the Output structure
        output.outfile->cd();
        output.histograms_1d[string_hash("h_nPVs")] = make_shared<TH1D>("h_nPVs", "nPVs", 50, 0, 50);
    
        h_nPVs = output.histograms_1d.at(string_hash("h_nPVs"));
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        h_nPVs->Fill(event.lc_int.get(string_hash("PV_npvsGood")));
    }
    const string getName() const override {
        return "EventVarsAnalyzer";
    }
};

class JetJetPair {
public:
    const Jet* j1;
    const Jet* j2;
    const double dr;
    JetJetPair(const Jet& _j1, const Jet& _j2, const double _dr) :
        j1(&_j1),
        j2(&_j2),
        dr(_dr) {
    }
};

//Here we compute as an example the deltaR between all jet pairs
class JetDeltaRAnalyzer : public Analyzer {
public:
    Output& output;

    shared_ptr<TH1D> h_deltaR;
    JetDeltaRAnalyzer(Output& _output) : output(_output) {
        cout << "Creating JetDeltaRAnalyzer" << endl;
        output.outfile->cd();
        output.histograms_1d[string_hash("h_deltaR")] = make_shared<TH1D>("h_deltaR", "Jet pair dR", 100, 0, 6);
    
        h_deltaR = output.histograms_1d.at(string_hash("h_deltaR"));
    }

    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        const auto njets = event.jets.size();
    
        //we would like to store a list of all the jet pairs along with their delta R
        //This allows us to find, for example, the two closest jets.
        vector<JetJetPair> jet_drpairs;
      
        //Creating the TLorentzVector is math-heavy, so we do it only once outside the jet-jet loop
        vector<TLorentzVector> jet_lvs;
        for (auto jet : event.jets) {
            jet_lvs.push_back(make_lv(jet.pt(), jet.eta(), jet.phi(), jet.mass()));
        }
    
        //Loop over jet pairs
        for (unsigned int i = 0; i < njets; i++) {
            for (unsigned int j = i + 1; j < njets; j++) {
                const auto& lv1 = jet_lvs.at(i);
                const auto& lv2 = jet_lvs.at(j);
    
                const auto dr = lv1.DeltaR(lv2);
                jet_drpairs.push_back(JetJetPair(event.jets.at(i), event.jets.at(j), dr));
                h_deltaR->Fill(dr);
            }
        }
    }

    const string getName() const override {
        return "JetDeltaRAnalyzer";
    }
};


class LeptonPairAnalyzer : public Analyzer {
public:
    Output& output;

    LeptonPairAnalyzer(Output& _output) : output(_output) {};

    virtual void analyze(NanoEvent& _event) override {

        auto& event = static_cast<Event&>(_event);

	
        vector<LazyObject*> leps;
        for (const auto& lep : event.muons) {
            leps.push_back((LazyObject*)&lep);
        }
        for (const auto& lep : event.electrons) {
            leps.push_back((LazyObject*)&lep);
        }
        
        vector<TLorentzVector> lep_lvs;
        for (const auto* lep : leps) {
            lep_lvs.push_back(make_lv(lep->pt(), lep->eta(), lep->phi(), lep->mass()));
        }

        double lep2_highest_inv_mass = 0;
        for (auto il1 = 0; il1 < leps.size(); il1++) {
            for (auto il2 = il1 + 1; il2 < leps.size(); il2++) {

                const auto& lv1 = lep_lvs.at(il1);
                const auto& lv2 = lep_lvs.at(il2);
                
                const auto& lv_tot = lv1 + lv2;
                const auto inv_mass = lv_tot.M();

                if (inv_mass > lep2_highest_inv_mass) {
                    lep2_highest_inv_mass = inv_mass;
                }
            }
        }
        event.lep2_highest_inv_mass = lep2_highest_inv_mass;
    };
    
    virtual const string getName() const override {
        return "LeptonPairAnalyzer";
    };
};

class MyTreeAnalyzer : public TreeAnalyzer {
public:
    float lep2_highest_inv_mass;
    
    MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {
        out_tree->Branch("lep2_highest_inv_mass", &lep2_highest_inv_mass, "lep2_highest_inv_mass/F");
    }
 
    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        lep2_highest_inv_mass = event.lep2_highest_inv_mass;
        //lep2_highest_inv_mass = 0.0;
        TreeAnalyzer::analyze(event);
    }

    const string getName() const override {
        return "MyTreeAnalyzer"; 
    }
};

//This is the main event loop
//Given a TTreeReader reader, we process all the specified analyzers and store the
//output in the Output data structure.
//You shouldn't have to add anything to the event loop if you want to compute a new
//quantity - rather, you can add a new Analyzer
FileReport looper_main(
    const Configuration& config,
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

    //We initialize the C++ representation of the event (data row) from the TTreeReader
    Event event(reader, config);

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


#endif
