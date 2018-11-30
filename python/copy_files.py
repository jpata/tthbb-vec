#!/usr/bin/python
# -*- coding: utf-8 -*-
##
# Example : copy a set of files in one go
#
from __future__ import print_function
import optparse
import shlex
import sys
import argparse
import logging
import os
from analysis import Analysis

try:
    import gfal2
except ImportError as e:
    print("gfal2-python library not found", e)

def event_callback(event):
    if event.domain == "GFAL2:CORE:COPY" and event.stage == "LIST:ITEM": 
    	print("[%s] %s %s %s" % (event.timestamp, event.domain, event.stage, event.description))

def monitor_callback(src, dst, average, instant, transferred, elapsed):
    print("[%4d] %.2fMB (%.2fKB/s)\r" % (elapsed, transferred/1014/1024, average/1024))
    sys.stdout.flush()

def check_target_dir(dest):
    if "file://" in dest:
        dest = dest.replace("file://", "")
        dest_dir = os.path.dirname(dest)
        if not os.path.exists(dest_dir):
            os.makedirs(dest_dir)

def copy_files(sources, destinations, overwrite=False, validate=False):
    # Instantiate gfal2
    ctx = gfal2.creat_context()

    # Set transfer parameters
    params = ctx.transfer_parameters()
    params.event_callback = event_callback
    params.monitor_callback = monitor_callback
    params.create_parent = True
    params.nbstreams = 5 
    params.overwrite = overwrite
    params.checksum_check = validate

    #make sure target directories exist    
    for dest in destinations:
        check_target_dir(dest)
 
    # Copy!
    # In this case, an exception will be thrown if the whole process fails
    # If any transfer fail, the method will return a list of GError objects, one per file
    # being None if that file succeeded
    try:
        errors = ctx.filecopy(params, sources, destinations)
        if not errors:
            print("Copy succeeded!")
        else:
            for i in range(len(errors)):
                e = errors[i]
                src = sources[i]
                dst = destinations[i]
                if e:
                    print("%s => %s failed [%d] %s" % (src, dst, e.code, e.message))
                else:
                    print("%s => %s succeeded!" % (src, dst))
    except Exception, e:
        print("Copy failed: %s" % str(e))
        sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='The nanoflow analysis controller')
    parser.add_argument('-a','--analysis', help='The analysis yaml file',
        required=False, default="./data/analysis.yaml", action="store",
    )
    parser.add_argument('-l','--loglevel', help='The logging level',
        required=False, default="WARNING", action="store",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"]
    )
    parser.add_argument('-d','--datasets', help='The datasets to process',
        required=False, action="append", default=None
    )
    args = parser.parse_args()
    logging.basicConfig(level=getattr(logging, args.loglevel))
    analysis = Analysis.from_yaml(args.analysis)
   
    for ds in analysis.mc_datasets + analysis.data_datasets:
        fns = ds.get_files()
        sources = [ds.global_file_prefix + fn for fn in fns]
        destinations = [ds.cache_location + fn for fn in fns]
        print("caching", ds.name)
        copy_files(sources, destinations, overwrite=False, validate=False)
