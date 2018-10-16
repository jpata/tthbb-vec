#!/usr/bin/env python
import ROOT
import json
import sys

def load_header(path):
    print "loading header {0}".format(path)
    ret = ROOT.gROOT.ProcessLine('#include "{0}"'.format(path))
    if ret != 0:
        raise Exception("Could not load header {0}".format(path))

def load_lib(path):
    ret = ROOT.gSystem.Load(path)
    if ret != 0:
        raise Exception("Could not load library {0}".format(path))

def setup():
    ROOT.gROOT.ProcessLine('.include interface')
   
    load_header("nanoflow.h") 
    load_header("myanalyzers.h") 
    
    load_lib("bin/libnanoflow.so")    

def FileReport_to_dict(p):
    r = {
        "filename": p.filename,
        "num_events_processed": p.num_events_processed,
        "cpu_time": p.cpu_time,
        "real_time": p.real_time,
        "speed": p.speed,
        "event_duration": p.event_duration,
        "analyzer_durations": list(p.analyzer_durations),
        "analyzer_names": list(p.analyzer_names)
    }
    return r

if __name__ == "__main__":
    setup()
    
    vector_Analyzer = getattr(ROOT, "vector<Analyzer*>")
    looper_main = getattr(ROOT, "looper_main")

    input_json = sys.argv[1]
    output_json = sys.argv[2]
    
    conf = ROOT.Configuration(input_json)
    conf.use_jets = False
    output = ROOT.Output(conf.output_filename)
    
    analyzers = vector_Analyzer()

    jetevent_analyzer = ROOT.JetEventAnalyzer(output)
    muonevent_analyzer = ROOT.MuonEventAnalyzer(output)
    electronevent_analyzer = ROOT.ElectronEventAnalyzer(output)
    sumpt_analyzer = ROOT.SumPtAnalyzer(output)
    eventvars_analyzer = ROOT.EventVarsAnalyzer(output)
    leppair_analyzer = ROOT.LeptonPairAnalyzer(output)
    tree_analyzer = ROOT.MyTreeAnalyzer(output)
    
    #analyzers.push_back(jetevent_analyzer)
    analyzers.push_back(muonevent_analyzer)
    analyzers.push_back(electronevent_analyzer)
    analyzers.push_back(leppair_analyzer)
    analyzers.push_back(sumpt_analyzer)
    analyzers.push_back(eventvars_analyzer)
    analyzers.push_back(tree_analyzer)

    all_reports = []    
    for inf in conf.input_files:
        tf = ROOT.TFile.Open(inf)
        reader = ROOT.TTreeReader("Events", tf)
        report = looper_main(conf, inf, reader, output, analyzers, -1)
        all_reports.append(report)
   
    output.close()
    
    with open(output_json, "w") as outfile:
        reports = [FileReport_to_dict(p) for p in all_reports]
        json.dump(reports, outfile, indent=2)
