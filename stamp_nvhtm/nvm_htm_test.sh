#!/bin/bash

# test[1]="./genome/genome -g16384 -s64 -n16777216 -r1 -t" 
# test[2]="./intruder/intruder -a10 -l128 -n262144 -s1 -r1 -t" 
# test[3]="./kmeans/kmeans -m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r1 -p" 
# test[4]="./kmeans/kmeans -m40 -n40 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r1 -p" 
# test[5]="./labyrinth/labyrinth  -i labyrinth/inputs/random-x512-y512-z7-n512.txt -t" 
# test[6]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -r1 -t" 
# test[7]="./vacation/vacation -n4 -q60 -u90 -r1048576 -t4194304 -a1 -c" 
# test[8]="./vacation/vacation  -n2 -q90 -u98 -r1048576 -t4194304 -a1 -c" 
# test[9]="./yada/yadome -g16384 -s64 -n16777216 -r1 -t" 
test[1]="./intruder/intruder -a10 -l32 -n32768 -s1 -r1 -t" 
test[2]="./genome/genome -g1024 -s64 -n8388608 -r1 -t" 
test[3]="./vacation/vacation -n2 -q20 -u10 -r262144 -a1 -t65536 -c"  # low
test[4]="./vacation/vacation -n4 -q50 -u30 -r262144 -a1 -t65536 -c"  # high
test[5]="./yada/yada -a15 -i ./yada/inputs/ttimeu10000.2 -r1 -t"
test[6]="./kmeans/kmeans -m40 -n40 -t0.00001 -r1 -i ./kmeans/inputs/random-n65536-d32-c16.txt -p" # low
test[7]="./kmeans/kmeans -m15 -n15 -t0.00001 -r1 -i ./kmeans/inputs/random-n65536-d32-c16.txt -p" # high
test[8]="./ssca2/ssca2 -s18 -i1.0 -u1.0 -l3 -p3 -r1 -t" 
test[9]="./labyrinth/labyrinth -i labyrinth/inputs/random-x256-y256-z3-n256.txt -t" 


testname=("" "intruder" "genome" "vacation-low" "vacation-high" "yada" "kmeans-low" "kmeans-high" "ssca2" "labyrinth")

bench_date=$(date +%m-%d_%H-%M)
function run_bench {

	# rm -f $1
    resdir="../res/stamp_$bench_date/$1"
    mkdir -p $resdir

    # note: ssca can't be used because of out of memory (exceeds max of mmap size)
	for i in 1 2 3 4 5 6 7 8 9 # 1 2 3 4 5 6 7 # 1 2 3 4 5 6 7 8 9
	do
        resdir_tmp="$resdir/${testname[$i]}"
        mkdir -p $resdir_tmp
		for t in 1 2 4 8 # 10 14 15 28 29 56 # 1 2 4 8
		do
			for a in `seq 3` # `seq 30`
			do
				echo timeout 1m ${test[$i]} $t
				timeout 1m ${test[$i]} $t > $resdir_tmp/time_thr${t}_trial${a} 2> $resdir_tmp/result_thr${t}_trial${a}
				if [[ $? -ne 0 ]] ; then 
					timeout 10m ${test[$i]} $t  # second try
				fi
			done
		done
		echo "${test[$i]}1:8 COMPLETE!\n" >> $1.txt
	done
}

if [ -z "$1" ]; then
    echo "error: no ppath!"
    exit
fi
if [ -z "$2" ]; then
    echo "warning: no vpath"
fi

# MAKEFILE_ARGS="SOLUTION=1" ./build-stamp.sh htm-sgl-nvm test_HTM.txt
# run_bench test_HTM
# 
# MAKEFILE_ARGS="SOLUTION=2" ./build-stamp.sh htm-sgl-nvm test_AVNI.txt
# run_bench test_AVNI
# 
# MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=1 LOG_SIZE=100000 THREASHOLD=0.1" ./build-stamp.sh htm-sgl-nvm test_REDO-COUNTER-LLOG-P.txt
# run_bench test_REDO-COUNTER-LLOG-P
# 
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=1 LOG_SIZE=10000000 PERIOD=1000" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-LLOG-P.txt
# run_bench test_REDO-TS-LLOG-P
# 
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=1 LOG_SIZE=10000 PERIOD=1000" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-SLOG-P.txt
# run_bench test_REDO-TS-SLOG-P
# 
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=2 LOG_SIZE=10000000 THREASHOLD=0.5" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-LLOG-R.txt
# run_bench test_REDO-TS-LLOG-R
# 
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=2 LOG_SIZE=10000 THREASHOLD=0.5" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-SLOG-R.txt
# run_bench test_REDO-TS-SLOG-R

# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=10000 stats=1 emulator=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=10000 THREASHOLD=0.5" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_emulator

# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=10000 stats=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=10000 THREASHOLD=0.5 USE_PMEM=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt

# クソでかログ
(cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 -j)
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
run_bench test_use_mmap

(cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 parallel_cp=1 -j)
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
run_bench test_para_cp

(cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 parallel_cp=1 log_compression=1 -j)
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 LOG_COMPRESSION=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
run_bench test_log_comp

# クソしょぼログ
# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=10000 stats=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=10000 THREASHOLD=0.5 USE_PMEM=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_use_mmap

# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=10000 stats=1 parallel_cp=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=10000 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_para_cp
# 
# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=10000 stats=1 parallel_cp=1 log_compression=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=10000 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 LOG_COMPRESSION=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_log_comp

# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 parallel_cp=1 task_dist=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 STAT=1 CHECK_TASK_DISTRIBUTION=1 PPATH=$1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_task_dist

# DRAM
if [ ! -z "$2" ]; then
(cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 -j)
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
run_bench test_use_mmap_dram
# 
# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 parallel_cp=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_para_cp
# 
# (cd ../; make type=nvhtm tree=bptree stats=1 dist-clean; make type=nvhtm tree=bptree logsize=41943328 stats=1 parallel_cp=1 log_compression=1 -j)
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=41943328 THREASHOLD=0.5 USE_PMEM=1 PARALLEL_CHECKPOINT=1 LOG_COMPRESSION=1 PPATH=$1 STAT=1" ./build-stamp.sh htm-sgl-nvm test_REDO-TS-FORK.txt
# run_bench test_log_comp
fi
