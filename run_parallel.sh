#!/bin/bash

find /fast/tmp1/jobs/dyjets_ll/ -name "*.json" | parallel --gnu -j16 LD_LIBRARY_PATH=./bin/:$LD_LIBRARY_PATH ./bin/looper {} {}.out
find /fast/tmp1/jobs/dyjets_ll/ -name "*.root" | xargs hadd -f /fast/tmp1/dyjets_ll.root
./bin/df /fast/tmp1/dyjets_ll.root out_dyjets.root

find /fast/tmp1/jobs/ggh_hmumu/ -name "*.json" | parallel --gnu -j16 LD_LIBRARY_PATH=./bin/:$LD_LIBRARY_PATH ./bin/looper {} {}.out
find /fast/tmp1/jobs/ggh_hmumu/ -name "*.root" | xargs hadd -f /fast/tmp1/ggh_hmumu.root
./bin/df /fast/tmp1/ggh_hmumu.root out_ggh.root
