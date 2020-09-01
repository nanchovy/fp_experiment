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
result_dir="logsize_csv"

mkdir -p ${result_dir}
for type in $types
do
    type_dir=`eval echo '$'${type}`
    for op in $ops
    do
        _logsz=`echo $logsz_list |  sed -E "s/^([0-9]+) .*/\1/"`
        thrs=`ls ${root_dir}/${plain}/logsz_${_logsz} | grep $op | cut -f 4 -d '.' | sort -n -u`
        echo $thrs
        echo $logsz_list
        for thr in $thrs
        do
            echo 'logsize,HTM-block,Checkpoint-block,Abort,End' > ${result_dir}/wait_${op}_${type}.thr.${thr}.csv
            for logsz in $logsz_list
            do
                logsz_dir=logsz_$logsz
                target_dir=`echo "${root_dir}/${type_dir}/${logsz_dir}"`
                dmp=`ls ${target_dir} | grep $op`
                cblock_time_sum=0
                hblock_time_sum=0
                ablock_time_sum=0
                end_time_sum=0
                for trial in $trials
                do
                    hblock_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "HTM " | cut -f 5 -d ' '`
                    cblock_time1=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "WAIT_MORE_LOG" | cut -f 7 -d ' '`
                    cblock_time2=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "CHECK_LOG_ABORT" | cut -f 7 -d ' '`
                    ablock_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "TRANSACTION_ABORT_TIME" | cut -f 2 -d ' '`
                    end_time=`cat ${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp | grep "wait_for" | tail -n 1 | cut -f 3 -d ' '`
                    hblock_time_sum=`echo "scale=7; ${hblock_time} + ${hblock_time_sum}" | bc`
                    cblock_time_sum=`echo "scale=7; ${cblock_time1} + ${cblock_time2} + ${cblock_time_sum}" | bc`
                    ablock_time_sum=`echo "scale=7; ${ablock_time} + ${ablock_time_sum}" | bc`
                    end_time_sum=`echo "scale=7; ${end_time} + ${end_time_sum}" | bc`
                done
                cbl_pt=`echo "scale=7; (${cblock_time_sum} / ${max_trial}) / ${thr}" | bc`
                hbl_pt=`echo "scale=7; (${hblock_time_sum} / ${max_trial}) / ${thr}" | bc`
                abl_pt=`echo "scale=7; (${ablock_time_sum} / ${max_trial}) / ${thr}" | bc`
                end_pt=`echo "scale=7; (${end_time_sum} / ${max_trial})" | bc`
                echo "$logsz,$hbl_pt,$cbl_pt,$abl_pt,$end_pt" >> ${result_dir}/wait_${op}_${type}.thr.${thr}.csv
            done
        done
    done
done
