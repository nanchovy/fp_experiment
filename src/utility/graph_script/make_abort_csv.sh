#!/bin/sh

if [ $# -eq 0 ]; then
    root_dir=.
else
    root_dir=$1
fi
ops='insert delete search'
types='bptree_concurrent_ fptree_concurrent_'
# memtypes='pmem vmem'
memtypes='pmem'
max_trial=3
trials=`seq 1 3`

mkdir -p abort
for memtype in $memtypes
do
    for type in $types
    do
        if ! [ -e ${root_dir}/${memtype}/count_abort/${type} ] ; then
            continue
        fi
        mkdir -p abort/$memtype/$type
        for op in $ops
        do
            echo 'thread,abort,conflict,capacity,explicit' > abort/$memtype/$type/abort_${op}.csv
            target_dir=`echo "${root_dir}/${memtype}/count_abort/${type}/logsz_65824"`
            thrs=`ls ${target_dir} | grep $op | cut -f 4 -d '.' | sort -n -u`
            for thr in $thrs
            do
                    other_tmp=0
                    conflict_tmp=0
                    capacity_tmp=0
                    explicit_tmp=0
                    for tri in $trials
                    do
                        tmp=`tail -n 6 ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | grep other | sed -E "s/.* ([0-9]+) times/\1/"`
                        other_tmp=`echo "scale=7; ${other_tmp} + ${tmp}" | bc`
                        tmp=`tail -n 6 ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | grep conflict | sed -E "s/.* ([0-9]+) times/\1/"`
                        conflict_tmp=`echo "scale=7; ${conflict_tmp} + ${tmp}" | bc`
                        tmp=`tail -n 6 ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | grep capacity | sed -E "s/.* ([0-9]+) times/\1/"`
                        capacity_tmp=`echo "scale=7; ${capacity_tmp} + ${tmp}" | bc`
                        tmp=`tail -n 6 ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | grep user | sed -E "s/.* ([0-9]+) times/\1/"`
                        explicit_tmp=`echo "scale=7; ${explicit_tmp} + ${tmp}" | bc`
                    done
                other=`echo "scale=7; ${other_tmp} / ${max_trial}" | bc`
                conflict=`echo "scale=7; ${conflict_tmp} / ${max_trial}" | bc`
                capacity=`echo "scale=7; ${capacity_tmp} / ${max_trial}" | bc`
                explicit=`echo "scale=7; ${explicit_tmp} / ${max_trial}" | bc`
                abort=`echo "scale=7; $explicit + $capacity + $conflict + $other" | bc`
                echo "$thr,$abort,$conflict,$capacity,$explicit" >> abort/$memtype/$type/abort_${op}.csv
            done
        done
    done
done
