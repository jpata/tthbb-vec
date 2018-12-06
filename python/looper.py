#!/usr/bin/env python
from __future__ import print_function
import json
import sys
import nanoflow

def setup_nanoflow():
    ROOT = nanoflow.import_ROOT()
    print("setting include dir")   
    ROOT.gROOT.ProcessLine('.include interface')
    print("including nanoflow.h")   
    nanoflow.load_header("nanoflow.h") 
    print("including demoanalysis.h")   
    nanoflow.load_header("demoanalysis.h")   

def run_looper(input_json, output_json):
    ROOT = nanoflow.import_ROOT()
    print("Constructing analysis")   
    an = nanoflow.SequentialAnalysis(input_json, getattr(ROOT, "looper_main_demoanalysis")) 

    print("Adding MuonEventAnalyzer")   
    an.add(ROOT.MuonEventAnalyzer(an.output))
    print("Adding MyTreeAnalyzer")   
    an.add(ROOT.MyTreeAnalyzer(an.output))
    
    print("Running analysis")   
    reports = an.run()
    an.save(reports, output_json)

def run_looper_args(args):
    run_looper(*args)

if __name__ == "__main__":
   
    setup_nanoflow()
    
    input_json = sys.argv[1]
    output_json = sys.argv[2]
   
    run_looper(input_json, output_json)
