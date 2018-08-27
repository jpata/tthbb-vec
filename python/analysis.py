import yaml
import subprocess
import os
import logging
import json

LOG_MODULE_NAME = logging.getLogger(__name__)

def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]

class Dataset:
    def __init__(self, name, process, file_prefix):
        self.name = name
        self.process = process
        self.file_prefix = file_prefix

    def __repr__(self):
        s = "Dataset(name={0})".format(self.name)
        return s

    def escape_name(self):
        return self.name.replace("/", "__")

    def cache_filename(self):
        return os.path.join("data", "das_cache", self.process, self.escape_name() + ".txt")
    
    def job_filename(self, njob):
        return os.path.join("data", "jobs", self.process, self.escape_name(), "job_{0}.json".format(njob))

    def cache_files(self):
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
        lines = [li.strip() for li in open(self.cache_filename(), "r").readlines()]
        return lines

    def lfn_to_pfn(self, fn):
        return self.file_prefix + fn

    def create_jobfiles(self, files_per_job):
        files = self.get_files()
        files = map(self.lfn_to_pfn, files)
        
        target_dir = os.path.dirname(self.job_filename(0))
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
       
        ijob = 0
        for files_chunk in chunks(files, files_per_job):
            with open(self.job_filename(ijob), "w") as fi:
                job_json = {
                    "input_filenames": files_chunk,
                    "output_filename": "out.root",
                }
                fi.write(json.dumps(job_json, indent=2))
            ijob += 1

class Analysis:
    def __init__(self, mc_datasets, data_datasets):
        self.mc_datasets = mc_datasets
        self.data_datasets = data_datasets

    @staticmethod
    def from_yaml(yaml_path):
        with open(yaml_path, 'r') as stream:
            data_loaded = yaml.load(stream)

            global_file_prefix = data_loaded["datasets"]["global_file_prefix"]

            mc_datasets = []
            for process_name in data_loaded["datasets"]["simulation"]:
                for ds in data_loaded["datasets"]["simulation"][process_name]:
                    mc_datasets.append(Dataset(ds["name"], process_name, global_file_prefix))
            
            data_datasets = []
            for process_name in data_loaded["datasets"]["data"]:
                for ds in data_loaded["datasets"]["data"][process_name]:
                    data_datasets.append(Dataset(ds["name"], process_name, global_file_prefix))

            return Analysis(mc_datasets, data_datasets)
   
    def cache_filenames(self):
        for dataset in self.mc_datasets:
            dataset.cache_files()
        for dataset in self.data_datasets:
            dataset.cache_files()

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    analysis = Analysis.from_yaml("data/analysis.yaml")
   
    for ds in analysis.mc_datasets:
        ds.create_jobfiles(2)
    for ds in analysis.data_datasets:
        ds.create_jobfiles(2) 
