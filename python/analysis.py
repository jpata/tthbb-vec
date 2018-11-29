#!/usr/bin/env python


#Hack to get ROOT to ignore command line arguments that we want
#to pass to Python
import sys
tmpargv = sys.argv
sys.argv = ['-b', '-n']
import ROOT

import yaml
import subprocess
import os
import logging
import json
import argparse
import glob
import multiprocessing


sys.argv[:] = tmpargv[:]

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
        return self.name.replace("/", "__")[2:]

    def cache_filename(self):
        """Summary
        
        Returns:
            TYPE: Description
        """
        return os.path.join(self.tmpdir, "das_cache", self.process, self.escape_name() + ".txt")
    
    def job_filename(self, njob):
        """Summary
        
        Args:
            njob (TYPE): Description
        
        Returns:
            TYPE: Description
        """
        return os.path.join(self.tmpdir, "jobs", self.process, self.escape_name(), "job_{0}.json".format(njob))

    def cache_files(self):
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

    def get_files(self):
        """Summary
        
        Returns:
            TYPE: Description
        """
        lines = [li.strip() for li in open(self.cache_filename(), "r").readlines()]
        return lines

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
        files = self.get_files()
        files = map(self.lfn_to_pfn, files)
        
        target_dir = os.path.dirname(self.job_filename(0))
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
       
        ijob = 0
        for files_chunk in chunks(files, files_per_job):
            with open(self.job_filename(ijob), "w") as fi:
		_outfile = "out.root"
		if not outfile:
		    _outfile = os.path.join(target_dir, "out_{0}.root".format(ijob))
                job_json = {
                    "input_filenames": files_chunk,
                    "output_filename": _outfile,
                    "max_events": -1
                }
                fi.write(json.dumps(job_json, indent=2))
            ijob += 1
        LOG_MODULE_NAME.info("{0} jobfiles created in {1}".format(ijob+1, target_dir))

    def get_jobfiles(self):
        target_dir = os.path.dirname(self.job_filename(0))
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
            cache_location = data_loaded["datasets"]["cache_location"]
            use_cache = data_loaded["datasets"]["use_cache"]
            tmpdir = data_loaded["datasets"]["workdirectory"]

            mc_datasets = []
            for process_name in data_loaded["datasets"]["simulation"]:
                for ds in data_loaded["datasets"]["simulation"][process_name]:
                    mc_datasets.append(Dataset(ds["name"], process_name, global_file_prefix, cache_location, use_cache, tmpdir))
            
            data_datasets = []
            for process_name in data_loaded["datasets"]["data"]:
                for ds in data_loaded["datasets"]["data"][process_name]:
                    data_datasets.append(Dataset(ds["name"], process_name, global_file_prefix, cache_location, use_cache, tmpdir))

            return Analysis(mc_datasets, data_datasets)
   
    def get_datasets(self):
        return self.mc_datasets + self.data_datasets

    def cache_filenames(self, datasets):
        """Summary
        """
        for ds in datasets:
            ds.cache_files()

    def copy_files(self, datasets):
        from copy_files import copy_files
        for ds in datasets:
            fns = ds.get_files()
            sources = [ds.global_file_prefix + fn for fn in fns]
            destinations = [ds.cache_location + fn for fn in fns]
            print "caching", ds.name
            copy_files(sources, destinations, overwrite=False, validate=False)


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
        from looper import run_looper, run_looper_args, setup_nanoflow
        setup_nanoflow()
        ijobs = 0
        args = []
        for ds in datasets:
            for jobfile in ds.get_jobfiles():
                args += [(jobfile, jobfile + ".out")]
        
        p = multiprocessing.Pool(num_procs)
        p.map(run_looper_args, args)
        p.close()  

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='The nanoflow analysis controller')
    parser.add_argument('-a','--analysis', help='The analysis yaml file',
        required=False, default="./data/analysis.yaml", action="store",

    )
    parser.add_argument('--cache_das', help='Caches the dataset filenames from DAS',
        action="store_true"
    )
    parser.add_argument('--copy_files', help='Copies the datasets locally to `cache_location`',
        action="store_true"
    )
    parser.add_argument('--remove_jobfiles', help='Removes the jobfiles from a previous run',
        action="store_true"
    )
    parser.add_argument('--create_jobfiles', help='Creates the jobfiles for the looper',
        action="store_true"
    )
    parser.add_argument('--run_jobs', help='Runs the analysis jobs',
        action="store_true"
    )
    parser.add_argument('-l','--loglevel', help='The logging level',
        required=False, default="INFO", action="store",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"]
    )
    args = parser.parse_args()


    logging.basicConfig(level=getattr(logging, args.loglevel))
    
    analysis = Analysis.from_yaml(args.analysis)
    
    datasets = analysis.get_datasets()

    if args.cache_das:
        analysis.cache_filenames(datasets)
    
    if args.copy_files:
        analysis.copy_files(datasets)

    if args.remove_jobfiles:
        analysis.remove_jobfiles(datasets)

    if args.create_jobfiles:
        analysis.create_jobfiles(datasets, 1)
    
    if args.run_jobs:
        analysis.run_jobs(datasets, 16)
