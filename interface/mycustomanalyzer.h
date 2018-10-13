#include "myanalyzers.h"
#include "nanoflow.h"

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

        double highest_inv_mass = 0;
        for (auto il1 = 0; il1 < leps.size(); il1++) {
            for (auto il2 = il1 + 1; il2 < leps.size(); il2++) {

                const auto& lv1 = lep_lvs.at(il1);
                const auto& lv2 = lep_lvs.at(il2);
                
                const auto& lv_tot = lv1 + lv2;
                const auto inv_mass = lv_tot.M();

                if (inv_mass > highest_inv_mass) {
                    highest_inv_mass = inv_mass;
                }
            }
        }
        event.highest_inv_mass = highest_inv_mass;
    };
    
    virtual const string getName() const override {
        return "LeptonPairAnalyzer";
    };
};

class MyTreeAnalyzer : public TreeAnalyzer {
public:
    float highest_inv_mass;
    
    MyTreeAnalyzer(Output& _output) : TreeAnalyzer(_output) {
        out_tree->Branch("highest_inv_mass", &highest_inv_mass, "highest_inv_mass/F");
    }
   
 
    void analyze(NanoEvent& _event) override {
        auto& event = static_cast<Event&>(_event);
        highest_inv_mass = event.highest_inv_mass;
        TreeAnalyzer::analyze(event);
    }

    const string getName() const override {
        return "MyTreeAnalyzer"; 
    }
};

