import yaml
import subprocess
import os

class Dataset:
    def __init__(self, name, process):
        self.name = name
        self.process = process

    def __repr__(self):
        s = "Dataset(name={0})".format(self.name)
        return s

    def escape_name(self):
        return self.name.replace("/", "__")

    def cache_filename(self):
        return os.path.join("data", self.process, self.escape_name() + ".txt")

    def cache_files(self):
        ret = subprocess.check_output('echo dasgoclient --query="file dataset={0}" --limit=0'.format(self.name), shell=True)

        target_dir = os.path.dirname(self.cache_filename())
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)

        with open(self.cache_filename(), "w") as fi:
            for line in ret.split("\n"):
                if line.endswith(".root"):
                    fi.write(line)

        return

    def get_files(self):
        lines = open(self.cache_filename(), "r").readlines()
        return lines


class Analysis:
    def __init__(self, mc_datasets, data_datasets):
        self.mc_datasets = mc_datasets
        self.data_datasets = data_datasets

    @staticmethod
    def from_yaml(yaml_path):
        with open(yaml_path, 'r') as stream:
            data_loaded = yaml.load(stream)

            mc_datasets = []
            for process_name in data_loaded["datasets"]["simulation"]:
                for ds in data_loaded["datasets"]["simulation"][process_name]:
                    mc_datasets.append(Dataset(ds["name"], process_name))
            
            data_datasets = []
            for process_name in data_loaded["datasets"]["simulation"]:
                for ds in data_loaded["datasets"]["simulation"][process_name]:
                    data_datasets.append(Dataset(ds["name"], process_name))

            return Analysis(mc_datasets, data_datasets)


analysis = Analysis.from_yaml("data/analysis.yaml")
analysis.mc_datasets[0].cache_files()
print analysis.mc_datasets[0].get_files()