#!/bin/bash
graphscr_dir=../../../src/utility/graph_script/
cd ../../../res/$1/
mkdir -p graph
cd graph
mkdir -p scaling/pmem
# mkdir -p scaling/vmem
# python3 ${graphscr_dir}/compare_tree_thread_time.py
# python3 ${graphscr_dir}/compare_tree_thread_time_part.py
# python3 ${graphscr_dir}/compare_tree_thread_time_presen.py
# python3 ${graphscr_dir}/compare_tree_thread_time_faw.py
# python3 ${graphscr_dir}/compare_tree_thread_time_hetero.py
# mkdir -p logsize/pmem
# mkdir -p logsize/vmem
# python3 ${graphscr_dir}/compare_tree_logsize_time.py
# 
# ${graphscr_dir}/make_abort_nvhtm_csv.sh ..
# python3 ${graphscr_dir}/compare_thread_logsz_nvhtm_abort.py
# python3 ${graphscr_dir}/compare_thread_logsz_nvhtm_total_abort.py
# python3 ${graphscr_dir}/compare_thread_logsz_nvhtm_total_abort_presen.py
# python3 ${graphscr_dir}/compare_thread_logsz_nvhtm_abort_presen.py
# 
# ${graphscr_dir}/make_abort_csv.sh ..
# python3 ${graphscr_dir}/compare_thread_logsz_abort.py
# 
# mkdir -p write_freq/pmem
# mkdir -p write_freq/vmem
# python3 ${graphscr_dir}/write_freq_graph.py ../pmem write_freq/pmem
# python3 ${graphscr_dir}/write_freq_graph.py ../vmem write_freq/vmem
# 
# mkdir -p wait_time/pmem
# mkdir -p wait_time/vmem
# ${graphscr_dir}/make_block_time_logsize_csv.sh ../pmem/elapsed_time
# mv logsize_csv wait_time/pmem
# ${graphscr_dir}/make_block_time_logsize_csv.sh ../vmem/elapsed_time
# mv logsize_csv wait_time/vmem
# ${graphscr_dir}/make_block_time_thread_csv.sh ../pmem/elapsed_time
# mv thread_csv wait_time/pmem
# ${graphscr_dir}/make_block_time_thread_csv.sh ../vmem/elapsed_time
# mv thread_csv wait_time/vmem

# mkdir -p checkpoint_time/pmem
# mkdir -p checkpoint_time/vmem
# ${graphscr_dir}/make_checkpoint_time_thread_csv.sh ../pmem/elapsed_time
# mv thread_csv checkpoint_time/pmem
# ${graphscr_dir}/make_checkpoint_time_thread_csv.sh ../vmem/elapsed_time
# mv thread_csv checkpoint_time/vmem
# mkdir -p checkpoint/pmem
# mkdir -p checkpoint/vmem
# ${graphscr_dir}/make_checkpoint_csv.sh ../pmem/elapsed_time
# mv thread_csv checkpoint/pmem
# ${graphscr_dir}/make_checkpoint_csv.sh ../vmem/elapsed_time
# mv thread_csv checkpoint/vmem

${graphscr_dir}/make_write_amount_csv.sh ..
${graphscr_dir}/make_write_amount_nvhtm_csv.sh ..
python3 ${graphscr_dir}/compare_tree_write_amount.py
