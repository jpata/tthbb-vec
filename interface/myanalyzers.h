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

    void analyze(Event& event) {
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

    virtual const string getName() const {
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

    void analyze(Event& event) {
        h_nPVs->Fill(event.lc_int.get(string_hash("PV_npvsGood")));
    }
    const string getName() const {
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

    void analyze(Event& event) {
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

    const string getName() const {
        return "JetDeltaRAnalyzer";
    }
};

TLorentzVector make_lv(float pt, float eta, float phi, float mass);

#endif
