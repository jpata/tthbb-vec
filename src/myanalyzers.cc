#include "myanalyzers.h"

// Jet SumPT analyzer
SumPtAnalyzer::SumPtAnalyzer(Output& _output) : output(_output) {

    //Create the sumpt histogram in the output data structure
    output.outfile->cd();
    output.histograms_1d[string_hash("h_sumpt")] = std::make_shared<TH1D>("h_sumpt", "sumpt", 100, 0, 1000);

    //Retrieve the pointer to the sumpt histogram for convenient access
    h_sumpt = output.histograms_1d.at(string_hash("h_sumpt"));
}

void SumPtAnalyzer::analyze(const Event& event) {
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


//Event variable analyzer
EventVarsAnalyzer::EventVarsAnalyzer(Output& _output) : output(_output) {

    //Create and store the nPV histogram in the Output structure
    output.outfile->cd();
    output.histograms_1d[string_hash("h_nPVs")] = std::make_shared<TH1D>("h_nPVs", "nPVs", 50, 0, 50);

    h_nPVs = output.histograms_1d.at(string_hash("h_nPVs"));
}

void EventVarsAnalyzer::analyze(const Event& event) {
    h_nPVs->Fill(event.lc_int.get(string_hash("PV_npvsGood")));
}

TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
    TLorentzVector lv;
    lv.SetPtEtaPhiM(pt, eta, phi, mass);
    return lv;
}


//Jet Delta-R analyzer
JetDeltaRAnalyzer::JetDeltaRAnalyzer(Output& _output) : output(_output) {
    output.outfile->cd();
    output.histograms_1d[string_hash("h_deltaR")] = std::make_shared<TH1D>("h_deltaR", "Jet pair dR", 100, 0, 6);

    h_deltaR = output.histograms_1d.at(string_hash("h_deltaR"));
}

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

void JetDeltaRAnalyzer::analyze(const Event& event) {
    const auto njets = event.jets.size();

    //we would like to store a list of all the jet pairs along with their delta R
    //This allows us to find, for example, the two closest jets.
    std::vector<JetJetPair> jet_drpairs;
  
    //Creating the TLorentzVector is math-heavy, so we do it only once outside the jet-jet loop
    std::vector<TLorentzVector> jet_lvs;
    for (auto jet : event.jets) {
        jet_lvs.push_back(make_lv(jet.pt(), jet.eta(), jet.phi(), jet.mass()));
    }

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