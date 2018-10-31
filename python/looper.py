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

def setup_nanoflow():
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

class SequentialAnalysis:

    def __init__(self, input_json):
        self.modules = []
        
        self.conf = ROOT.Configuration(input_json)
        self.output = ROOT.Output(self.conf.output_filename)
    
        vector_Analyzer = getattr(ROOT, "vector<Analyzer*>")
        self.analyzers = vector_Analyzer()

    def add(self, module):
        self.modules.append(module)

    def run(self):
        looper_main = getattr(ROOT, "looper_main")
        all_reports = []

        for module in self.modules:
            self.analyzers.push_back(module)
  
        for inf in self.conf.input_files:
            tf = ROOT.TFile.Open(inf)
            reader = ROOT.TTreeReader("Events", tf)
            report = looper_main(self.conf, inf, reader, self.output, self.analyzers, -1, 100)
            all_reports.append(report)

        self.output.close()
        
        reports = [FileReport_to_dict(p) for p in all_reports]
	return reports
   
    def save(self, reports, output_json): 
        with open(output_json, "w") as outfile:
            json.dump(reports, outfile, indent=2)


def run_looper(input_json, output_json):
    an = SequentialAnalysis(input_json) 

    an.add(ROOT.MuonEventAnalyzer(an.output))
    an.add(ROOT.MatrixElementEventAnalyzer(an.output, 13000))
    #an.add(ROOT.JetEventAnalyzer(an.output))
    #an.add(ROOT.GenJetEventAnalyzer(an.output))
    #an.add(ROOT.GenRecoJetMatchAnalyzer(an.output))
    #an.add(ROOT.ElectronEventAnalyzer(an.output))
    #an.add(ROOT.GenLeptonEventAnalyzer(an.output))
    #an.add(ROOT.GenRecoLeptonMatchAnalyzer(an.output))
    # an.add(ROOT.SumPtAnalyzer(an.output))
    #an.add(ROOT.EventVarsAnalyzer(an.output))
    # an.add(ROOT.LeptonPairAnalyzer(an.output))
    # an.add(ROOT.JetDeltaRAnalyzer(an.output))
    an.add(ROOT.MyTreeAnalyzer(an.output))
    
    reports = an.run()
    an.save(reports, output_json)

def run_looper_args(args):
    run_looper(*args)

if __name__ == "__main__":
   
    setup_nanoflow()
    
    input_json = sys.argv[1]
    output_json = sys.argv[2]
   
    run_looper(input_json, output_json)
