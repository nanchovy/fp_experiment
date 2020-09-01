#!/bin/sh

if [ $# -eq 0 ]; then
    root_dir=.
else
    root_dir=$1
fi
plain=bptree_nvhtm_0
db=bptree_nvhtm_1
logsz_list=`ls ${root_dir}/${plain} | sed -e "s/logsz_//" | sort -n`
# types='plain db'
types='plain'
ops='insert delete search'
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
            echo 'thread,reserve,commit-finding,apply-log,flush' > ${result_dir}/checkpoint_${op}_${type}.logsize.${logsz}.csv
            logsz_dir=logsz_$logsz
            target_dir=`echo "${root_dir}/${type_dir}/${logsz_dir}"`
            thrs=`ls ${target_dir} | grep $op | cut -f 4 -d '.' | sort -n -u`
            for thr in $thrs
            do
                rcheck_time_sum=0
                ccheck_time_sum=0
                acheck_time_sum=0
                fcheck_time_sum=0
                for trial in $trials
                do
                    rcheck_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "sec. 1" | cut -f 6 -d ' '`
                    ccheck_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "sec. 2" | cut -f 7 -d ' '`
                    acheck_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "sec. 3" | cut -f 6 -d ' '`
                    fcheck_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "sec. 4" | cut -f 6 -d ' '`
                    rcheck_time_sum=`echo "scale=7; ${rcheck_time} + ${rcheck_time_sum}" | bc`
                    ccheck_time_sum=`echo "scale=7; ${ccheck_time} + ${ccheck_time_sum}" | bc`
                    acheck_time_sum=`echo "scale=7; ${acheck_time} + ${acheck_time_sum}" | bc`
                    fcheck_time_sum=`echo "scale=7; ${fcheck_time} + ${fcheck_time_sum}" | bc`
                done
                rcheck_time_avg=`echo "scale=7; (${rcheck_time_sum} / ${max_trial})" | bc`
                ccheck_time_avg=`echo "scale=7; (${ccheck_time_sum} / ${max_trial})" | bc`
                acheck_time_avg=`echo "scale=7; (${acheck_time_sum} / ${max_trial})" | bc`
                fcheck_time_avg=`echo "scale=7; (${fcheck_time_sum} / ${max_trial})" | bc`
                echo "$thr,$rcheck_time_avg,$ccheck_time_avg,$acheck_time_avg,$fcheck_time_avg" >> ${result_dir}/checkpoint_${op}_${type}.logsize.${logsz}.csv
            done
        done
    done
done
