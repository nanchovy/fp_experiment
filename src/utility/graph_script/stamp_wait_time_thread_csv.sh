#!/bin/sh

if [ $# -eq 0 ]; then
    root_dir=..
else
    root_dir=$1
fi
# exec_types='emulator log_comp para_cp use_mmap'
exec_types='log_comp para_cp use_mmap'
bench_types='genome intruder kmeans-high kmeans-low labyrinth ssca2 vacation-high vacation-low yada'
max_trial=3
trials=`seq 1 ${max_trial}`
result_dir="./waittime"
thrs="1 2 4 8"

function make_csv () {
    bench_type=$1
    source_dir=$2
    target_dir=$3

    echo 'thread,HTM-block,Checkpoint-block,Abort' > ${target_dir}/wait_${bench_type}.csv
    source_dir_tmp=`echo "$source_dir/$bench_type"`
    for thr in $thrs
    do
        cblock_time_sum=0
        hblock_time_sum=0
        ablock_time_sum=0
        end_time_sum=0
        for trial in $trials
        do
            # echo "${target_dir}/${op}_concurrent.exe.thr.${thr}.trial.${trial}.dmp"
            hblock_time=`cat ${source_dir_tmp}/result_thr${thr}_trial${trial} | grep "HTM " | cut -f 5 -d ' '`
            # echo "hblock_time = ${hblock_time}"
            cblock_time1=`cat ${source_dir_tmp}/result_thr${thr}_trial${trial} | grep "d by WAIT_MORE_LOG" | cut -f 7 -d ' '`
            # echo "cblock_time1 = ${cblock_time1}"
            cblock_time2=`cat ${source_dir_tmp}/result_thr${thr}_trial${trial} | grep "d by CHECK_LOG_ABORT" | cut -f 7 -d ' '`
            # echo "cblock_time2 = ${cblock_time2}"
            ablock_time=`cat ${source_dir_tmp}/result_thr${thr}_trial${trial} | grep "TRANSACTION_ABORT_TIME" | cut -f 2 -d ' '`
            # echo "ablock_time = ${ablock_time}"
            # end_time=`cat ${target_dir}/result_thr${thr}_trial${trial} | grep "wait_for" | tail -n 1 | cut -f 3 -d ' '`
            # echo "end_time = ${end_time}"
            hblock_time_sum=`echo "scale=7; ${hblock_time} + ${hblock_time_sum}" | bc`
            cblock_time_sum=`echo "scale=7; ${cblock_time1} + ${cblock_time2} + ${cblock_time_sum}" | bc`
            ablock_time_sum=`echo "scale=7; ${ablock_time} + ${ablock_time_sum}" | bc`
            # end_time_sum=`echo "scale=7; ${end_time} + ${end_time_sum}" | bc`
        done
        hbl_pt=`echo "scale=7; (${hblock_time_sum} / ${max_trial}) / ${thr}" | bc`
        cbl_pt=`echo "scale=7; (${cblock_time_sum} / ${max_trial}) / ${thr}" | bc`
        abl_pt=`echo "scale=7; (${ablock_time_sum} / ${max_trial}) / ${thr}" | bc`
        # end_pt=`echo "scale=7; (${end_time_sum} / ${max_trial})" | bc`
        # echo "end = ${end_pt}"
        # echo "$thr,$hbl_pt,$cbl_pt,$abl_pt,$end_pt" >> ${result_dir}/wait_${b}.csv
        echo "$thr,$hbl_pt,$cbl_pt,$abl_pt" >> ${target_dir}/wait_${bench_type}.csv
    done
}

function apply_all_bench () {
    exec_type=$1
    root_dir=$2
    result_dir=$3
    mkdir -p $result_dir/$exec_type
    for bench_type in $bench_types
    do
        make_csv $bench_type $root_dir/test_$exec_type $result_dir/$exec_type
    done
}

mkdir -p ${result_dir}
for exec_type in $exec_types
do
    apply_all_bench $exec_type $root_dir $result_dir
done
