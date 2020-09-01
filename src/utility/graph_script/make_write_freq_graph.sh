#!/bin/sh

wf_dir="write_freq"
memtypes="pmem vmem"

mkdir -p $wf_dir
for memtype in $memtypes
do
    mkdir -p $wf_dir/$memtype
    python3 write_freq_graph.py $1/$memtype $wf_dir/$memtype
done
