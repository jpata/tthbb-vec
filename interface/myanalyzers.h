//In this file, we declare our own custom analyzers, based on the interface defined in nanoflow.h
#ifndef MYANALYZERS_H
#define MYANALYZERS_H

#include <TLorentzVector.h>

#include "nanoflow.h"

//This example Analyzer computes the sum(pt) of all the jets, muons and electrons in the event
//and stores it in a histogram. The actual implementation code is located in myanalyzers.cc,
//so we can compile it separately.
class SumPtAnalyzer : public Analyzer {
public:
    Output& output;

    //We store a pointer to the output histogram distribution here in order to address it conveniently
    //The histogram itself is stored in the Output data structure
    std::shared_ptr<TH1D> h_sumpt;
    SumPtAnalyzer(Output& _output);

    virtual void analyze(const Event& event);
};


//This example Analyzer simply saves the number of primary vertices in an output histogram
class EventVarsAnalyzer : public Analyzer {
public:
    Output& output;

    //Convenient short-hand access to the PV histogram
    std::shared_ptr<TH1D> h_nPVs;
    EventVarsAnalyzer(Output& _output);

    virtual void analyze(const Event& event);
};

//Here we compute as an example the deltaR between all jet pairs
class JetDeltaRAnalyzer : public Analyzer {
public:
    Output& output;

    std::shared_ptr<TH1D> h_deltaR;
    JetDeltaRAnalyzer(Output& _output);

    virtual void analyze(const Event& event);
};

#endif