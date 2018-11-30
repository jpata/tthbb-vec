#!/usr/bin/env python
from __future__ import print_function

#Hack to get ROOT to ignore command line arguments that we want
#to pass to Python
import sys
tmpargv = sys.argv
sys.argv = ['-b', '-n']
import ROOT
sys.argv[:] = tmpargv[:]

import yaml
import subprocess
import logging
import json
import argparse
import glob
import multiprocessing
import optparse
import shlex
import os

LOG_MODULE_NAME = logging.getLogger(__name__)

def chunks(l, n):
    """Yield successive n-sized chunks from l.
    
    Args:
        l (list): List from which chunks are retrieved
        n (number): Number of objects per chunk
    
    Yields:
        list of lists: input list as n-size chunks
    """
    for i in range(0, len(l), n):
        yield l[i:i + n]

class Dataset:

    """Datatype that represents a DAS dataset
    
    Attributes:
        global_file_prefix (string): The ROOT TFile prefix that allows to open an LFN (/store/...)
        name (string): The DAS name of the dataset
        process (string): The nickname for the physics process that this dataset belongs to
    """
    
    def __init__(self, name, process, global_file_prefix, cache_location, use_cache, tmpdir):
        """Summary
        
        Args:
            name (string): The DAS name of the dataset
            process (string): The nickname for the physics process that this dataset belongs to
            global_file_prefix (string): The ROOT TFile prefix that allows to open an LFN (/store/...)
            cache_location (string): The location of the local file cache
            use_cache (boolean): If true, access files from cache_location instead of global_file_prefix in jobs
        """
        self.name = name
        self.process = process
        self.global_file_prefix = global_file_prefix
        self.cache_location = cache_location
        self.use_cache = use_cache
        self.tmpdir = tmpdir
        self.files = None

    def __repr__(self):
        """
        
        Returns:
            string: The string representation of the Dataset
        """
        s = "Dataset(name={0})".format(self.name)
        return s

    def escape_name(self):
        """Removes any slashes and other characters from the name such that it can be used as a filename
        
        Returns:
            string: The DAS name usable as a filename
        """
        name = self.name.replace("/", "__")
        if name.startswith("__"):
            name = name[2:]
        return name

    def get_das_cache_filename(self):
        """Summary
        
        Returns:
            TYPE: Description
        """
        return os.path.join(self.tmpdir, "das_cache", self.process, self.escape_name() + ".txt")
    
    def get_job_filename(self, njob):
        """Summary
        
        Args:
            njob (TYPE): Description
        
        Returns:
            TYPE: Description
        """
        return os.path.join(self.tmpdir, "jobs", self.process, self.escape_name(), "job_{0}.json".format(njob))

    def cache_filenames(self):
        """Summary
        
        Returns:
            TYPE: Description
        """
        LOG_MODULE_NAME.info("caching dataset {0}".format(self.name))
        ret = subprocess.check_output('dasgoclient --query="file dataset={0}" --limit=0'.format(self.name), shell=True)

        target_dir = os.path.dirname(self.cache_filename())
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
        
        nfiles = 0
        with open(self.cache_filename(), "w") as fi:
            for line in ret.split("\n"):
                if line.endswith(".root"):
                    fi.write(line + "\n")
                    nfiles += 1
        
        LOG_MODULE_NAME.info("retrieved {0} files from DAS".format(nfiles))

        return

    def get_filenames(self):
        """Summary
        
        Returns:
            TYPE: Description
        """
        if self.files is None:
            lines = [li.strip() for li in open(self.get_das_cache_filename(), "r").readlines()]
            self.files = lines
        return self.files

    def lfn_to_pfn(self, fn):
        """Summary
        
        Args:
            fn (TYPE): Description
        
        Returns:
            TYPE: Description
        """
        pref = self.global_file_prefix
        if self.use_cache:
            pref = self.cache_location

        return pref + fn

    def create_jobfiles(self, files_per_job, outfile=None):
        """Summary
        
        Args:
            files_per_job (TYPE): Description
        """
        LOG_MODULE_NAME.info("creating job files for {0}".format(self))
        files = self.get_filenames()
        files = map(self.lfn_to_pfn, files)
        
        target_dir = os.path.dirname(self.get_job_filename(0))
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
       
        ijob = 0
        for files_chunk in chunks(files, files_per_job):
            with open(self.get_job_filename(ijob), "w") as fi:
		_outfile = "out.root"
		if not outfile:
		    _outfile = os.path.join(target_dir, "out_{0}.root".format(ijob))
                job_json = {
                    "input_filenames": files_chunk,
                    "output_filename": _outfile,
                    "max_events": -1,
                    "report_period": 10000,
                }
                fi.write(json.dumps(job_json, indent=2))
            ijob += 1
        LOG_MODULE_NAME.info("{0} job files created in {1}".format(ijob, target_dir))

    def get_jobfiles(self):
        target_dir = os.path.dirname(self.get_job_filename(0))
        if not os.path.exists(target_dir):
   	    raise Exception("Job files not created, call create_jobfiles() first")
        return glob.glob(os.path.join(target_dir, "job_*.json"))

    def remove_jobfiles(self):
        jobfiles = self.get_jobfiles()
        map(os.remove, jobfiles)
    
class Analysis:

    """Summary
    
    Attributes:
        data_datasets (TYPE): Description
        mc_datasets (TYPE): Description
    """
    
    def __init__(self, mc_datasets, data_datasets):
        """Summary
        
        Args:
            mc_datasets (TYPE): Description
            data_datasets (TYPE): Description
        """
        self.mc_datasets = mc_datasets
        self.data_datasets = data_datasets
    
    @staticmethod
    def sanitize_cache_location(path):
        if not path.startswith("file://"):
            path = "file://" + os.getcwd() + "/" + path
        return path

    @staticmethod
    def from_yaml(yaml_path):
        """Summary
        
        Args:
            yaml_path (TYPE): Description
        
        Returns:
            TYPE: Description
        """
        with open(yaml_path, 'r') as stream:
            LOG_MODULE_NAME.info("loading analysis from {0}".format(yaml_path)) 
            data_loaded = yaml.load(stream)

            global_file_prefix = data_loaded["datasets"]["global_file_prefix"]
            cache_location = Analysis.sanitize_cache_location(data_loaded["datasets"]["cache_location"])
            use_cache = data_loaded["datasets"]["use_cache"]
            tmpdir = data_loaded["datasets"]["workdirectory"]

            mc_datasets = []
            for process_name in data_loaded["datasets"]["simulation"]:
                for ds in data_loaded["datasets"]["simulation"][process_name]:
                    dataset = Dataset(
                        ds["name"],
                        process_name,
                        global_file_prefix,
                        cache_location,
                        use_cache, tmpdir
                    )
                    if "files" in ds.keys() and len(ds["files"]) > 0:
                        dataset.files = ds["files"] 
                    mc_datasets.append(dataset)
            
            data_datasets = []
            for process_name in data_loaded["datasets"]["data"]:
                for ds in data_loaded["datasets"]["data"][process_name]:
                    dataset = Dataset(
                        ds["name"],
                        process_name,
                        global_file_prefix,
                        cache_location,
                        use_cache, tmpdir
                    )
                    if "files" in ds.keys() and len(ds["files"]) > 0:
                        dataset.files = ds["files"] 
                    data_datasets.append(dataset)

            return Analysis(mc_datasets, data_datasets)
   
    def get_datasets(self):
        return self.mc_datasets + self.data_datasets

    def cache_das_filenames(self, datasets):
        """Summary
        """
        for ds in datasets:
            ds.cache_das_filenames()

    def copy_files(self, datasets):
        for ds in datasets:
            LOG_MODULE_NAME.info("copying files for dataset {0} from remote storage {1} to local cache {2}".format(
                ds.name, ds.global_file_prefix, ds.cache_location
            ))
            fns = ds.get_filenames()
            LOG_MODULE_NAME.debug("dataset {0} has {1} files".format(ds.name, len(fns)))
            sources = [ds.global_file_prefix + fn for fn in fns]
            destinations = [ds.cache_location + fn for fn in fns]
            copy_files(sources, destinations, overwrite=True, validate=False)


    def remove_jobfiles(self, datasets):
        for ds in datasets:
            ds.remove_jobfiles()

    def create_jobfiles(self, datasets, perjob):
        """Summary
        
        Args:
            perjob (int): Number of files to process per job
        """
        for ds in datasets:
            ds.create_jobfiles(perjob)

    def run_jobs(self, datasets, num_procs):
        
        ijobs = 0
        args = []
        for ds in datasets:
            for jobfile in ds.get_jobfiles():
                args += [(jobfile, jobfile + ".out")]
        
        #p = multiprocessing.Pool(num_procs, initializer=pool_init)
        map(self.run_looper_args, args)
        #p.close()  

    def __str__(self):
        s = "Analysis({0} MC datasets, {1} real data datasets)".format(len(self.mc_datasets), len(self.data_datasets))
        return s



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
    import gfal2
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

def load_header(path):
    print("loading header {0}".format(path))
    ret = ROOT.gROOT.ProcessLine('#include "{0}"'.format(path))
    if ret != 0:
        raise Exception("Could not load header {0}".format(path))

def load_lib(path):
    ret = ROOT.gSystem.Load(path)
    if ret != 0:
        raise Exception("Could not load library {0}".format(path))


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

    def __init__(self, input_json, looper_main):
        self.modules = []
        
        self.conf = ROOT.nanoflow.Configuration(input_json)
        self.output = ROOT.nanoflow.Output(self.conf.output_filename)
        print(self.conf, self.output)

        vector_Analyzer = getattr(ROOT, "std::vector<nanoflow::Analyzer*>")
        self.analyzers = vector_Analyzer()
        print(self.analyzers)
        
        self.looper_main = looper_main

    def add(self, module):
        self.modules.append(module)

    def run(self):
        all_reports = []

        for module in self.modules:
            self.analyzers.push_back(module)
  
        for inf in self.conf.input_files:
            tf = ROOT.TFile.Open(inf)
            reader = ROOT.TTreeReader("Events", tf)
            report = self.looper_main(self.conf, inf, reader, self.output, self.analyzers, self.conf.max_events, self.conf.report_period)
            all_reports.append(report)

        self.output.close()
        
        reports = [FileReport_to_dict(p) for p in all_reports]
	return reports
   
    def save(self, reports, output_json): 
        with open(output_json, "w") as outfile:
            json.dump(reports, outfile, indent=2)

