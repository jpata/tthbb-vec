from __future__ import print_function
import ROOT
import sys
import logging

LOG_MODULE_NAME = logging.getLogger(__name__)
def skim_with_selection(inpath, intree, outpath, selection, nevents):
    LOG_MODULE_NAME.info("Skimming {0}:{1} to {2}".format(inpath, intree, outpath))
    tf = ROOT.TFile(inpath)
    tt = tf.Get(intree) 
    outfile = ROOT.TFile(outpath, "RECREATE")
    outtree = tt.CopyTree(selection, "", nevents, 0)
    nentries = outtree.GetEntries()
    outfile.Write()
    outfile.Close()
    return nentries


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("skim.py inpath intree outpath nevents")
        sys.exit(0)

    inpath = sys.argv[1]    
    intree = sys.argv[2]    
    outpath = sys.argv[3]    
    nevents = int(sys.argv[4])
    skim_with_selection(inpath, intree, outpath, "", nevents)    
