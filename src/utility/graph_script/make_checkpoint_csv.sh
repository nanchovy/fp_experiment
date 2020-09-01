#!/bin/sh

if [ $# -eq 0 ]; then
    root_dir=.
else
    root_dir=$1
fi
plain=bptree_nvhtm_0
# db=bptree_nvhtm_1
ca=bptree_nvhtm_1
logsz_list=`ls ${root_dir}/${plain} | sed -e "s/logsz_//" | sort -n`
# types='plain db'
types='plain'
# types='plain ca'
ops='insert delete search'
# max_trial=5
max_trial=3
trials=`seq 1 ${max_trial}`
result_dir="thread_csv"

mkdir -p ${result_dir}
for type in $types
do
    type_dir=`eval echo '$'${type}`
    for op in $ops
    do
        for logsz in $logsz_list
        do
            echo 'thread,checkpoint' > ${result_dir}/checkpoint_${op}_${type}.logsize.${logsz}.csv
            logsz_dir=logsz_$logsz
            target_dir=`echo "${root_dir}/${type_dir}/${logsz_dir}"`
            thrs=`ls ${target_dir} | grep $op | cut -f 4 -d '.' | sort -n -u`
            for thr in $thrs
            do
                checkpoint_sum=0
                for trial in $trials
                do
                    checkpoint=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "] Nb. check" | cut -f 4 -d ' '`
                    checkpoint_sum=`echo "${checkpoint} + ${checkpoint_sum}" | bc`
                done
                checkpoint_avg=`echo "scale=7; (${checkpoint_sum} / ${max_trial})" | bc`
                echo "$thr,$checkpoint_avg" >> ${result_dir}/checkpoint_${op}_${type}.logsize.${logsz}.csv
            done
        done
    done
done
