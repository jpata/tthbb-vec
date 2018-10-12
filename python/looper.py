import ROOT
import json
import sys

def setup():
    ROOT.gROOT.ProcessLine('.include interface')
    ROOT.gROOT.ProcessLine('#include "nanoflow.h"')
    ROOT.gROOT.ProcessLine('#include "myanalyzers.h"')
    ROOT.gROOT.ProcessLine('#include "mycustomanalyzer.h"')
    ROOT.gSystem.Load('libnanoflow.so')

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

    input_json = sys.argv[1]
    output_json = sys.argv[2]
    
    conf = ROOT.Configuration(input_json)
    output = ROOT.Output(conf.output_filename)
    
    analyzers = vector_Analyzer()

    sumpt_analyzer = ROOT.SumPtAnalyzer(output)
    eventvars_analyzer = ROOT.EventVarsAnalyzer(output)
    leppair_analyzer = ROOT.LeptonPairAnalyzer(output)
    tree_analyzer = ROOT.MyTreeAnalyzer(output)
    
    analyzers.push_back(sumpt_analyzer)
    analyzers.push_back(eventvars_analyzer)
    analyzers.push_back(leppair_analyzer)
    analyzers.push_back(tree_analyzer)

    all_reports = []    
    for inf in conf.input_files:
        tf = ROOT.TFile.Open(inf)
        reader = ROOT.TTreeReader("Events", tf)
        report = ROOT.looper_main(inf, reader, output, analyzers, 100000)
        all_reports.append(report)
   
    output.close()
    
    with open(output_json, "w") as outfile:
        reports = [FileReport_to_dict(p) for p in all_reports]
        json.dump(reports, outfile, indent=2)
