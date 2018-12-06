#!/usr/bin/env python
import argparse, logging
import nanoflow

def main():
    parser = argparse.ArgumentParser(description='The nanoflow analysis controller, use `nanoflow.py -h` to get more information.')
    parser.add_argument('-a','--analysis', help='The analysis yaml file, e.g. data/analysis.yaml',
        required=True, action="store",
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
    
    analysis = nanoflow.Analysis.from_yaml(args.analysis)
    print(analysis)
        
    datasets = analysis.get_datasets()

    if args.cache_das:
        analysis.cache_das_filenames(datasets)
    
    if args.copy_files:
        analysis.copy_files(datasets)

    if args.remove_jobfiles:
        analysis.remove_jobfiles(datasets)

    if args.create_jobfiles:
        analysis.create_jobfiles(datasets, 1)
    
    if args.run_jobs:
        nanoflow.import_ROOT()
        from looper import setup_nanoflow, run_looper_args
        analysis.run_looper_args = run_looper_args
        analysis.run_jobs(datasets, 16, setup_nanoflow)

if __name__ == "__main__":
    main()
