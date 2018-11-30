#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
SKIMS="skim_Run2012B_DoubleElectron.root skim_Run2012C_DoubleElectron.root skim_SMHiggsToZZTo4L.root skim_ZZTo4e.root skim_Run2012B_DoubleMuParked.root skim_Run2012C_DoubleMuParked.root skim_ZZTo2e2mu.root skim_ZZTo4mu.root"

for f in $SKIMS; do
    curl -k https://hep.kbfi.ee/~joosep/cms_opendata_2012_nanoaod_nanoflow_skim/$f -o $DIR/$f
done
sleep 5
du -csh $DIR/*.root
