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
result_dir="./bench_stats"
thrs="1 2 4 8"

function format_switch () {
    source_dir=$1
    bench_type=$2
    thr=$3
    trial=$4
    case $bench_type in
        genome        ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        intruder      ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Elapsed" | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        kmeans-high   ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        kmeans-low    ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        labyrinth     ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Elapsed" | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        vacation-high ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        vacation-low  ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        ssca2         ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Time"    | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
        yada          ) cat ${source_dir}/time_thr${thr}_trial${trial} | grep "Elapsed" | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/' ;;
    esac
}

function calc_txsize () {
    target_file=$1
    write_num=`cat $target_file | grep TOTAL_WRITES | cut -f 11 -d ' '`
    tx_num=`cat $target_file | grep COMMIT | cut -f 3 -d ' '`
    echo "scale=9;$write_num/$tx_num" | bc
}

function calc_txratio () {
    target_dir=$1
    bench_type=$2
    whole_time=`format_switch $target_dir $bench_type 1 1`
    tx_time=`cat $target_dir/result_thr1_trial1| grep TRANSACTION_TIME | cut -f 2 -d ' '`
    echo "scale=9;$tx_time/$whole_time" | bc
}

function calc_writeratio () {
    target_dir=$1
    bench_type=$2
    whole_time=`format_switch $target_dir $bench_type 1 1`
    write_num=`cat $target_dir/result_thr1_trial1 | grep TOTAL_WRITES | cut -f 11 -d ' '`
    echo "scale=9;$write_num/$whole_time" | bc
}

function add_csv () {
    bench_type=$1
    source_dir=$2
    target_csv=$3

    echo $bench_type
    thr=1
    source_dir_tmp=`echo "$source_dir/$bench_type"`
    time_sum=0
    txsize=`calc_txsize $source_dir_tmp/result_thr${thr}_trial1`
    txratio=`calc_txratio $source_dir_tmp $bench_type`
    writeratio=`calc_writeratio $source_dir_tmp $bench_type`
    echo "$bench_type,$txsize,$txratio,$writeratio" >> ${target_csv}
}

function apply_all_bench () {
    exec_type=$1
    root_dir=$2
    result_dir=$3
    mkdir -p $result_dir/$exec_type
    echo 'bench_type,txsize,txratio,writeratio' > ${result_dir}/${exec_type}/stats.csv
    for bench_type in $bench_types
    do
        add_csv $bench_type $root_dir/test_$exec_type $result_dir/$exec_type/stats.csv
    done
}

mkdir -p ${result_dir}
for exec_type in $exec_types
do
    echo $exec_type
    apply_all_bench $exec_type $root_dir $result_dir
done
