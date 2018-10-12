import distributed
import tempfile
import subprocess
import json
import sys
import os
import glob
import time
import argparse
import logging

from analysis import Analysis

LOG_MODULE_NAME = logging.getLogger(__name__)

def run_looper(tup):
    infile, outfile = tup

    cmd = ["./looper", infile, outfile]
    print cmd
    env = os.environ
    p = subprocess.Popen(cmd, env=env, stdout=subprocess.PIPE, shell=False)
    (stdoutdata, stderrdata) = p.communicate()
    # p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    # (stdoutdata, stderrdata) = p.communicate()
    # print stdoutdata
    with open(outfile, "r") as fi:
        data = json.load(fi)
    return data

def main():
    parser = argparse.ArgumentParser(description='The nanoflow analysis controller')
    parser.add_argument('-a','--analysis', help='The analysis yaml file',
        required=False, default="./data/analysis.yaml", action="store",
    )
    parser.add_argument('-l','--loglevel', help='The logging level',
        required=False, default="INFO", action="store",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"]
    )
    parser.add_argument('-d','--datasets', help='The datasets to process',
        required=False, action="append", default=None
    )
    args = parser.parse_args()
    logging.basicConfig(level=getattr(logging, args.loglevel))
   
    analysis = Analysis.from_yaml(args.analysis)
    
    if not args.datasets:
        for dataset in analysis.mc_datasets:
            print dataset
	return
    for dataset in args.datasets:
	jobfiles = dataset.get_jobfiles()
        print jobfiles

if __name__ == "__main__":
    main()
    #print "creating Client"
    #from dask.distributed import Client, progress
    #cli = Client('127.0.0.1:8786')

    #print "calling map"
    #ret = cli.map(run_looper, [(fn, "out_{0}.json".format(ifn)) for ifn, fn in enumerate(fns)])

    #t1 = time.time()
    #print "calling gather"
    #progress(ret)
    #ret2 = cli.gather(ret)
    #for r in ret2:
    #    print r
    #t2 = time.time()
    #print t2 - t1

