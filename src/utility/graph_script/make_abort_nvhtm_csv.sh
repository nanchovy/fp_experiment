#!/bin/sh

if [ $# -eq 0 ]; then
    root_dir=.
else
    root_dir=$1
fi
plain=bptree_nvhtm_0
# db=bptree_nvhtm_1
ca=bptree_nvhtm_1
logsz_list=`ls ${root_dir}/pmem/elapsed_time/${plain} | sed -e "s/logsz_//" | sort -n`
# types='plain db'
# types='plain'
types='plain'
ops='insert delete search'
memtypes="pmem vmem"
max_trial=3
trials=`seq 1 ${max_trial}`

mkdir -p abort
for memtype in $memtypes
do
    for type in $types
    do
        type_dir=`eval echo '$'${type}`
        mkdir -p abort/$memtype/$type_dir
        for op in $ops
        do
            echo 'type,logsize,thread,abort,conflict,capacity,explicit' > abort/$memtype/$type_dir/abort_${op}.csv
            for logsz in $logsz_list
            do
                logsz_dir=logsz_$logsz
                target_dir=`echo "${root_dir}/${memtype}/elapsed_time/${type_dir}/${logsz_dir}"`
                thrs=`ls ${target_dir} | grep $op | cut -f 4 -d '.' | sort -n -u`
                for thr in $thrs
                do
                    abort_tmp=0
                    conflict_tmp=0
                    capacity_tmp=0
                    explicit_tmp=0
                    for tri in $trials
                    do
                        tmp=`grep "ABORTS :" ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | cut -f 3 -d ' '`
                        abort_tmp=`echo "scale=7; ${abort_tmp} + ${tmp}" | bc`
                        tmp=`grep "CONFLS :" ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | cut -f 3 -d ' '`
                        conflict_tmp=`echo "scale=7; ${conflict_tmp} + ${tmp}" | bc`
                        tmp=`grep "CAPACS :" ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | cut -f 3 -d ' '`
                        capacity_tmp=`echo "scale=7; ${capacity_tmp} + ${tmp}" | bc`
                        tmp=`grep "EXPLIC :" ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${tri}.dmp | cut -f 3 -d ' '`
                        explicit_tmp=`echo "scale=7; ${explicit_tmp} + ${tmp}" | bc`
                    done
                    abort=`echo "scale=7; ${abort_tmp} / ${max_trial}" | bc`
                    conflict=`echo "scale=7; ${conflict_tmp} / ${max_trial}" | bc`
                    capacity=`echo "scale=7; ${capacity_tmp} / ${max_trial}" | bc`
                    explicit=`echo "scale=7; ${explicit_tmp} / ${max_trial}" | bc`
                    echo "$type,$logsz,$thr,$abort,$conflict,$capacity,$explicit" >> abort/$memtype/$type_dir/abort_${op}.csv
                done
            done
        done
    done
done
