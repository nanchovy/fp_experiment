#include "log.h"
#ifdef USE_PMEM
extern void *pmem_pool;
#endif

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <set>
#include <list>
#include <thread>
#include <mutex>
#include <vector>
#include <unistd.h>
#ifdef USE_PMEM
#include <xmmintrin.h>
#endif
#ifdef PARALLEL_CHECKPOINT
#include <pthread.h>
#endif

using namespace std;

// pos has an hint, return the next tx after the given ts
static ts_s max_tx_after(NVLog_s *log, int start, ts_s target, int *pos)
{
  int hint = *pos;
  for (; hint != start; hint = ptr_mod_log(hint, -1)) {
    ts_s ts = entry_is_ts(log->ptr[hint]);
    if (ts && ts <= target) {
      *pos = ptr_mod_log(hint, -1); // updates for the next write
      return ts;
    }
  }
  *pos = start; // did not find
  return 0;
}

// pos has an hint PER LOG (array), return the idx of the next log to apply
static int max_next_log(int *pos, int starts[], int ends[])
{
  int i;
  ts_s max_ts[TM_nb_threads];
  ts_s disc_max;
  int disc_max_pos;
  int count_ends = 0;
  for (i = 0; i < TM_nb_threads; ++i) {
    max_ts[i] = 0;
    NVLog_s *log = NH_global_logs[i];
    ts_s ts = entry_is_ts(log->ptr[pos[i]]);
    while (!ts && pos[i] != starts[i]) {
      pos[i] = ptr_mod_log(pos[i], -1);
      ts = entry_is_ts(log->ptr[pos[i]]);
    }
    if (pos[i] == starts[i]) {
      count_ends++;
    }
    if (starts[i] != ends[i]) {
      max_ts[i] = ts;
    }
  }
  if (count_ends == TM_nb_threads) {
    return -1; // no more transactions
  }
  disc_max = max_ts[0];
  disc_max_pos = 0;
  for (i = 1; i < TM_nb_threads; ++i) {
    if (disc_max < max_ts[i]) {
      // some ts may be 0 if log ended
      disc_max = max_ts[i];
      disc_max_pos = i;
    }
  }
  return disc_max_pos;
}

#ifdef PARALLEL_CHECKPOINT
// static NVLog_s *log_g = NULL;
// NH_global_logsでいい
static int *starts_g = NULL;
static int *ends_g = NULL;
static int *pos_g = NULL;
static ts_s target_ts_g = 0;
static int finished_threads = 0;

void LOG_checkpoint_backward_start_thread(int thread_num) {
    int i;
    for (i = 0; i < thread_num; i++) {
        sem_post(&cp_back_sem);
    }
}

void LOG_checkpoint_backward_wait_for_thread(int thread_num) {
    sem_wait(&cpthread_finish_sem);
    finished_threads = 0;
#ifdef CHECK_TASK_DISTRIBUTION
    double entries_max = applied_entries[0];
    double entries_min = applied_entries[0];
    for (int i = 0; i < thread_num; i++) {
        if (applied_entries[i] > entries_max) {
            entries_max = applied_entries[i];
        }
        if (applied_entries[i] < entries_min) {
            entries_min = applied_entries[i];
        }
    }
    if (entries_min == 0) entries_min++;
    fprintf(stderr, "TASK DISTRIBUTION: %lf\n", entries_max/entries_min);
    for (int i = 0; i < thread_num; i++) {
        applied_entries[i] = 0;
    }
#endif
}

int LOG_checkpoint_backward_parallel_apply(int thread_num, int starts[], int ends[], int pos[], ts_s target_ts) {
  int i;
  if (starts_g == NULL) {
      starts_g = (int *)malloc(sizeof(int) * TM_nb_threads);
  }
  for (i = 0; i < TM_nb_threads; i++) {
      starts_g[i] = starts[i];
  }
  if (ends_g == NULL) {
      ends_g = (int *)malloc(sizeof(int) * TM_nb_threads);
  }
  for (i = 0; i < TM_nb_threads; i++) {
      ends_g[i] = ends[i];
  }
  if (pos_g == NULL) {
      pos_g = (int *)malloc(sizeof(int) * TM_nb_threads);
  }
  for (i = 0; i < TM_nb_threads; i++) {
      pos_g[i] = pos[i];
  }
  target_ts_g = target_ts;
  LOG_checkpoint_backward_start_thread(thread_num);
  LOG_checkpoint_backward_wait_for_thread(thread_num);
}

// void LOG_checkpoint_backward_parallel_apply(intptr_t assigned_addr, intptr_t mask) {
void LOG_checkpoint_backward_thread_apply(int thread_id, int number_of_threads) {
  int i, next_log, pos_local[TM_nb_threads];
  NVLog_s *log;
  ts_s target_ts;
#ifdef STAT
  struct timespec stt, edt;
  double time_tmp = 0;
#endif
  
  unordered_map<GRANULE_TYPE*, CL_BLOCK> writes_map;
#ifdef LOG_COMPRESSION
  NVLog_s *compressed_log = NH_global_compressed_logs[thread_id];
  int compressed_log_end = 0;
#endif
  vector<GRANULE_TYPE*> writes_list;
  writes_list.reserve(32000/number_of_threads);
  writes_map.reserve(32000/number_of_threads);
  // printf("thread %d begins to wait\n", thread_id);
  sem_wait(&cp_back_sem);
  // printf("thread %d started\n", thread_id);

  for (i = 0; i < TM_nb_threads; i++) {
      pos_local[i] = pos_g[i];
  }
  target_ts = target_ts_g;

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

#ifdef LOG_COMPRESSION
  int end_tmp = 0;
  MN_write(&compressed_log->end, &end_tmp, sizeof(compressed_log_end), 1);
  MN_flush(&compressed_log->end, sizeof(compressed_log_end), 1);
#endif

#ifndef MEASURE_PART_CP
  do {
    next_log = max_next_log(pos_local, starts_g, ends_g);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts = max_tx_after(log, starts_g[next_log], target_ts, &(pos_local[next_log])); // updates the ptr
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos_local[next_log] != starts_g[next_log]) {
      pos_local[next_log] = ptr_mod_log(pos_local[next_log], -1);
    } else {
      // ended
      break;
    }

    // within the start/end boundary
    // assert((starts_g[i] <= ends_g[i] && pos_local[i] >= starts_g[i] &&
    //   pos[i] <= ends[i]) || (starts[i] > ends[i] &&
    //     ((pos[i] >= 0 && pos[i] < starts[i]) ||
    //     (pos[i] < LOG_local_state.size_of_log && pos[i] > ends[i]))
    //   )
    // );

#  ifdef STAT
   // clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
    NVLogEntry_s entry = log->ptr[pos_local[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos_local[next_log] != starts_g[next_log]) {
#ifdef NUMBER_OF_ENTRIES
        read_entries[thread_id]++;
#endif

#  ifdef STAT
#    ifdef WRITE_AMOUNT_NVHTM
        // no_filter_write_amount += sizeof(entry.value);
#    endif
#  endif
#ifdef USE_PMEM
        // if ((uintptr_t)entry.addr < (uintptr_t)pmem_pool || (uintptr_t)((char *)pmem_pool + pmem_size) < (uintptr_t)entry.addr) {
        //     fprintf(stderr, "out of range\n");
        //     continue;
        // }
#endif
        // uses only the bits needed to identify the cache line
        intptr_t cl_addr = (((intptr_t)entry.addr >> 6) << 6);
        int val_idx = ((intptr_t)entry.addr & 0x38) >> 3; // use bits 4,5,6
        char bit_map = 1 << val_idx;
#ifdef LARGE_DIV
        if (thread_id * (pmem_size/number_of_threads) <= ((uintptr_t)entry.addr - (uintptr_t)pmem_pool) &&
                ((uintptr_t)entry.addr - (uintptr_t)pmem_pool) < (thread_id+1) * (pmem_size/number_of_threads)) {
#else
        if ((cl_addr >> 8) % number_of_threads == thread_id) {
#endif
#ifdef CHECK_TASK_DISTRIBUTION
            applied_entries[thread_id]++;
#endif
            auto it = writes_map.find((GRANULE_TYPE*)cl_addr);
            // printf("processing: %p by thread%d\n", cl_addr, thread_id);
            if (it == writes_map.end()) {
#ifdef NUMBER_OF_ENTRIES
                wrote_entries[i]++;
#endif
                // not found the write --> insert it
                CL_BLOCK block;
                block.bit_map = bit_map;
                /*block.block[val_idx] = entry.value;*/
                auto to_insert = make_pair((GRANULE_TYPE*)cl_addr, block);
                writes_map.insert(to_insert);
                writes_list.push_back((GRANULE_TYPE*)cl_addr);
#  ifdef LOG_COMPRESSION
                MN_write(&compressed_log->ptr[compressed_log_end], &entry, sizeof(NVLogEntry_s), 1);
                // MN_flush(&compressed_log->ptr[compressed_log_end], sizeof(NVLogEntry_s), 1);
                compressed_log_end++;
                if (compressed_log_end >= compressed_log->size_of_log) {
                    fprintf(stderr, "log compression:too much entries\n");
                    exit(1);
                }
#  else
                MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
#  endif
            } else {
                if ( !(it->second.bit_map & bit_map) ) {
#ifdef NUMBER_OF_ENTRIES
                    wrote_entries[i]++;
#endif
#  ifdef LOG_COMPRESSION
                    MN_write(&compressed_log->ptr[compressed_log_end], &entry, sizeof(NVLogEntry_s), 1);
                    // MN_flush(&compressed_log->ptr[compressed_log_end], sizeof(NVLogEntry_s), 1); // flushタイミングは要検討
                    compressed_log_end++;
                    if (compressed_log_end >= compressed_log->size_of_log) {
                        fprintf(stderr, "log compression:too much entries\n");
                        exit(1);
                    }
#  else
                    // Need to write this word
                    MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
#  endif
                    it->second.bit_map |= bit_map;
#  ifdef FAW_CHECKPOINT
                    if (it->second.bit_map == -1) {
                        MN_flush(it->first, CACHE_LINE_SIZE, 1);
                    }
#  endif
                }
            }
        }

        pos_local[next_log] = ptr_mod_log(pos_local[next_log], -1);
        entry = log->ptr[pos_local[next_log]];
        ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);
#ifdef LOG_COMPRESSION
  int flush_unit = CACHE_LINE_SIZE / sizeof(NVLogEntry_s);
  for (i = 0; i < compressed_log_end; i += flush_unit) {
      MN_flush(&compressed_log->ptr[i], flush_unit * sizeof(NVLogEntry_s), 1);
  }
#endif
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[0][thread_id] += time_tmp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
#else
  // ----------------------------------normal---------------------------------------------------------
  ts_s target_ts_tmp = target_ts;
  int pos_local_tmp[TM_nb_threads];
  for (int pos_i = 0; pos_i < TM_nb_threads; pos_i++) {
      pos_local_tmp[pos_i] = pos_local[pos_i];
  }
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
  do {
    next_log = max_next_log(pos_local_tmp, starts_g, ends_g);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts_tmp = max_tx_after(log, starts_g[next_log], target_ts_tmp, &(pos_local_tmp[next_log])); // updates the ptr
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos_local_tmp[next_log] != starts_g[next_log]) {
      pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
    } else {
      // ended
      break;
    }

    NVLogEntry_s entry = log->ptr[pos_local_tmp[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos_local_tmp[next_log] != starts_g[next_log]) {
#ifdef USE_PMEM
        if ((uintptr_t)entry.addr < (uintptr_t)pmem_pool || (uintptr_t)((char *)pmem_pool + pmem_size) < (uintptr_t)entry.addr) {
            // fprintf(stderr, "out of range\n");
            continue;
        }
#endif

        // uses only the bits needed to identify the cache line
        intptr_t cl_addr = (((intptr_t)entry.addr >> 6) << 6);
        int val_idx = ((intptr_t)entry.addr & 0x38) >> 3; // use bits 4,5,6
        char bit_map = 1 << val_idx;
        if ((cl_addr >> 8) % number_of_threads == thread_id) {
            auto it = writes_map.find((GRANULE_TYPE*)cl_addr);
            if (it == writes_map.end()) {
                // not found the write --> insert it
                CL_BLOCK block;
                block.bit_map = bit_map;
                /*block.block[val_idx] = entry.value;*/
                auto to_insert = make_pair((GRANULE_TYPE*)cl_addr, block);
                writes_map.insert(to_insert);
                writes_list.push_back((GRANULE_TYPE*)cl_addr);
#  ifdef LOG_COMPRESSION
                MN_write(&compressed_log->ptr[compressed_log_end], &entry, sizeof(NVLogEntry_s), 1);
                // MN_flush(&compressed_log->ptr[compressed_log_end], sizeof(NVLogEntry_s), 1);
                compressed_log_end++;
                if (compressed_log_end >= compressed_log->size_of_log) {
                    fprintf(stderr, "log compression:too much entries\n");
                    exit(1);
                }
#  else
                MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
#  endif
            } else {
                if ( !(it->second.bit_map & bit_map) ) {
#  ifdef LOG_COMPRESSION
                    MN_write(&compressed_log->ptr[compressed_log_end], &entry, sizeof(NVLogEntry_s), 1);
                    // MN_flush(&compressed_log->ptr[compressed_log_end], sizeof(NVLogEntry_s), 1); // flushタイミングは要検討
                    compressed_log_end++;
                    if (compressed_log_end >= compressed_log->size_of_log) {
                        fprintf(stderr, "log compression:too much entries\n");
                        exit(1);
                    }
#  else
                    // Need to write this word
                    MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
#  endif
                    it->second.bit_map |= bit_map;
#  ifdef FAW_CHECKPOINT
                    if (it->second.bit_map == -1) {
                        MN_flush(it->first, CACHE_LINE_SIZE, 1);
                    }
#  endif
                }
            }
        }

        pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
        entry = log->ptr[pos_local_tmp[next_log]];
        ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);
#ifdef LOG_COMPRESSION
  int flush_unit = CACHE_LINE_SIZE / sizeof(NVLogEntry_s);
  for (i = 0; i < compressed_log_end; i += flush_unit) {
      MN_flush(&compressed_log->ptr[i], flush_unit * sizeof(NVLogEntry_s), 1);
  }
#endif
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[0][thread_id] += time_tmp;
#  endif
  // ----------------------------------read only---------------------------------------------------------
  target_ts_tmp = target_ts;
  for (int pos_i = 0; pos_i < TM_nb_threads; pos_i++) {
      pos_local_tmp[pos_i] = pos_local[pos_i];
  }
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
  do {
    next_log = max_next_log(pos_local_tmp, starts_g, ends_g);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts_tmp = max_tx_after(log, starts_g[next_log], target_ts_tmp, &(pos_local_tmp[next_log])); // updates the ptr
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos_local_tmp[next_log] != starts_g[next_log]) {
      pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
    } else {
      // ended
      break;
    }

    NVLogEntry_s entry = log->ptr[pos_local_tmp[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos_local_tmp[next_log] != starts_g[next_log]) {
        pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
        entry = log->ptr[pos_local_tmp[next_log]];
        ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[2][thread_id] += time_tmp;
#  endif
  // ----------------------------------no write---------------------------------------------------------
  target_ts_tmp = target_ts;
  for (int pos_i = 0; pos_i < TM_nb_threads; pos_i++) {
      pos_local_tmp[pos_i] = pos_local[pos_i];
  }
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
  do {
    next_log = max_next_log(pos_local_tmp, starts_g, ends_g);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts_tmp = max_tx_after(log, starts_g[next_log], target_ts_tmp, &(pos_local_tmp[next_log])); // updates the ptr
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos_local_tmp[next_log] != starts_g[next_log]) {
      pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
    } else {
      // ended
      break;
    }

    NVLogEntry_s entry = log->ptr[pos_local_tmp[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos_local_tmp[next_log] != starts_g[next_log]) {

        // uses only the bits needed to identify the cache line
        intptr_t cl_addr = (((intptr_t)entry.addr >> 6) << 6);
        int val_idx = ((intptr_t)entry.addr & 0x38) >> 3; // use bits 4,5,6
        char bit_map = 1 << val_idx;
        if ((cl_addr >> 8) % number_of_threads == thread_id) {
            auto it = writes_map.find((GRANULE_TYPE*)cl_addr);
            if (it == writes_map.end()) {
                // not found the write --> insert it
                CL_BLOCK block;
                block.bit_map = bit_map;
                /*block.block[val_idx] = entry.value;*/
                auto to_insert = make_pair((GRANULE_TYPE*)cl_addr, block);
                writes_map.insert(to_insert);
                writes_list.push_back((GRANULE_TYPE*)cl_addr);
            } else {
                if ( !(it->second.bit_map & bit_map) ) {
                    it->second.bit_map |= bit_map;
                }
            }
        }

        pos_local_tmp[next_log] = ptr_mod_log(pos_local_tmp[next_log], -1);
        entry = log->ptr[pos_local_tmp[next_log]];
        ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[3][thread_id] += time_tmp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#  endif
#endif

#ifndef LOG_COMPRESSION
  // flushes the changes
  auto cl_iterator = writes_list.begin();
  // auto cl_it_end = writes_list.end();
  for (; cl_iterator != writes_list.end(); ++cl_iterator) {
    // TODO: must write the cache line, now is just spinning
    GRANULE_TYPE *addr = *cl_iterator;
#ifdef USE_PMEM
    MN_flush(addr, CACHE_LINE_SIZE, 1);
#else
    MN_flush(addr, CACHE_LINE_SIZE, 0);
#endif
  }
#ifdef USE_PMEM
  _mm_sfence();
#endif
#else
#ifdef USE_PMEM
  _mm_sfence();
#endif
  MN_write(&compressed_log->end, &compressed_log_end, sizeof(compressed_log_end), 1);
  MN_flush(&compressed_log->end, sizeof(compressed_log_end), 1);
#endif

#ifdef STAT
#  ifndef LOG_COMPRESSION
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[1][thread_id] += time_tmp;
#  endif
#endif
  int res = __sync_fetch_and_add(&finished_threads, 1);
  if (res == number_of_threads - 1) {
      sem_post(&cpthread_finish_sem);
  }

#ifdef LOG_COMPRESSION
  int applying_log = 0;
  while (applying_log < compressed_log_end) {
      NVLogEntry_s *entry = &compressed_log->ptr[applying_log];
      MN_write(entry->addr, &(entry->value), sizeof(GRANULE_TYPE), 1);
      applying_log++;
  }
  // flushes the changes
  auto cl_iterator = writes_list.begin();
  // auto cl_it_end = writes_list.end();
  for (; cl_iterator != writes_list.end(); ++cl_iterator) {
    // TODO: must write the cache line, now is just spinning
    GRANULE_TYPE *addr = *cl_iterator;
#ifdef USE_PMEM
    MN_flush(addr, CACHE_LINE_SIZE, 1);
#else
    MN_flush(addr, CACHE_LINE_SIZE, 0);
#endif
  }
#  ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  parallel_checkpoint_section_time_thread[1][thread_id] += time_tmp;
#  endif
#ifdef USE_PMEM
  _mm_sfence();
#endif
#endif
}

void *LOG_checkpoint_backward_thread_func(void *args) {
  checkpoint_args_s *cp_args = (checkpoint_args_s *)args;
  // printf("cp thread started: id = %d, num of threads = %d\n", cp_args->thread_id, cp_args->number_of_threads);
  LOG_local_state.size_of_log = NH_global_logs[0]->size_of_log;
  while (1) {
    LOG_checkpoint_backward_thread_apply(cp_args->thread_id, cp_args->number_of_threads);
  }
}
#endif

// Apply log backwards and avoid repeated writes
int LOG_checkpoint_backward_apply_one()
{
  // this function needs a huge refactoring!
  int i, j, targets[TM_nb_threads], size_hashmap = 0,
  starts[TM_nb_threads], ends[TM_nb_threads];
  int log_start, log_end;
  bool too_full = false;
  bool too_empty = false;
  bool someone_passed = false;
  int dist;
  NVLog_s *log;
#ifdef STAT
  struct timespec stt, edt;
  double time_tmp;
#endif

#ifndef PARALLEL_CHECKPOINT
  typedef struct _CL_BLOCK {
    char bit_map;
  } CL_BLOCK;
#endif

  sem_wait(NH_chkp_sem);
  *NH_checkpointer_state = 1; // doing checkpoint
  __sync_synchronize();

#ifndef PARALLEL_CHECKPOINT
  // stores the possible repeated writes
  unordered_map<GRANULE_TYPE*, CL_BLOCK> writes_map;
  vector<GRANULE_TYPE*> writes_list;
#endif

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

#ifndef PARALLEL_CHECKPOINT
  writes_list.reserve(32000);
  writes_map.reserve(32000);
#endif

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[0] += time_tmp;

  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

  // find target ts, and the idx in the log
  ts_s target_ts = 0;
  int pos[TM_nb_threads], pos_to_start[TM_nb_threads];

  // ---------------------------------------------------------------
  // First check if the logs are too empty
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    log_start = log->start;
    log_end = log->end;
    dist = distance_ptr(log_start, log_end);
    // sum_dist += dist;
    if (dist < TOO_EMPTY) {
      too_empty = true;
    }
    if (dist > TOO_FULL) {
      someone_passed = true;
      too_full = true;
    }
    if (dist > APPLY_BACKWARD_VAL) {
      j = ends[i];
      someone_passed = true;
    }
  }
  // Only apply log if someone passed the threshold mark
  if ((!too_full && too_empty) || !someone_passed) {
    *NH_checkpointer_state = 0; // doing checkpoint
#ifdef STAT
    if (*checkpoint_empty == 1) {
        *checkpoint_empty = 2;
    } else {
        *checkpoint_empty = 1;
    }
#ifdef NO_EMPTY_LOOP_TIME
    checkpoint_section_time[0] -= time_tmp;
#endif
#endif
    __sync_synchronize();
    return 1; // try again later
  }
#ifdef STAT
  *checkpoint_empty = 0;
#endif
  // ---------------------------------------------------------------

  // first find target_ts, then the remaining TSs
  // TODO: keep the minimum anchor
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];

    starts[i] = log->start;
    ends[i] = log->end;

    // TODO: check the size of the log
    size_t log_size = ptr_mod_log(ends[i], -starts[i]);
    ts_s ts = 0;
    // find target ts in this log
    if (log_size <= APPLY_BACKWARD_VAL) {
      j = ends[i];
    } else {
      j = ptr_mod_log(starts[i], APPLY_BACKWARD_VAL);
    }

    // TODO: repeated code
    for (; j != starts[i]; j = ptr_mod_log(j, -1)) {
      NVLogEntry_s entry = log->ptr[j];
      ts = entry_is_ts(entry);
      if (ts && ((ts < target_ts) || !target_ts)) { // find minimum
        target_ts = ts;
        break;
      }
    }

    // if ts == 0 it means that we need to analyze more log
    if (ts == 0) {
      j = ends[i];
      for (; j != starts[i]; j = ptr_mod_log(j, -1)) {
        NVLogEntry_s entry = log->ptr[j];
        ts = entry_is_ts(entry);
        if (ts && ((ts < target_ts) || !target_ts)) { // find minimum
          target_ts = ts;
          break;
        }
      }
    }

    targets[i] = j;
    size_hashmap += distance_ptr(starts[i], targets[i]);

    // targets[] is between starts[] and ends[]
    assert((starts[i] <= ends[i] && targets[i] >= starts[i] &&
      targets[i] <= ends[i]) || (starts[i] > ends[i] &&
        ((targets[i] >= 0 && targets[i] < starts[i]) ||
        (targets[i] < LOG_local_state.size_of_log &&
          targets[i] > ends[i])
        )
      )
    );
    // printf("s:%i t:%i e:%i \n", starts[i], targets[i], ends[i]);
  }

  // other TSs smaller than target
  // TODO: sweep from the threshold backward only
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    ts_s ts = 0;
    // find the maximum TS of other TXs smaller than target_ts

    // sizes[] has the target idx
    pos[i] = targets[i]; // init with some default value
    // if j == start (none TX has smaller TS) then pos_to_start must not be changed
    pos_to_start[i] = starts[i];

    for (j = pos[i]; j != starts[i]; j = ptr_mod_log(j, -1)) {
      NVLogEntry_s entry = log->ptr[j];
      ts = entry_is_ts(entry);
      if (ts && ts <= target_ts) {
        // previous idx contains the write
        pos[i] = j; // ptr_mod_log(j, -1);
        if(j == ends[i]) {
          pos_to_start[i] = j; // THERE IS SOME BUG HERE!
        } else {
          pos_to_start[i] = ptr_mod_log(j, 1); // THERE IS SOME BUG HERE!
        }
        break;
      }
    }
  }

  if (!target_ts) {
    *NH_checkpointer_state = 0; // doing checkpoint
    __sync_synchronize();
    return 1; // there isn't enough transactions
  }

  NH_nb_checkpoints = NH_nb_checkpoints + 1;
#ifdef STAT
  if (checkpoint_by_flags[2]) {
      checkpoint_by[2]++;
  } else if (checkpoint_by_flags[1]) {
      checkpoint_by[1]++;
  } else {
      checkpoint_by[0]++;
  }
  memset(checkpoint_by_flags, 0, sizeof(unsigned int) * 3);
#endif

  // find the write-set to apply to the checkpoint (Cache_lines!)
  int next_log;
  unsigned long long proc_writes = 0;

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[1] += time_tmp;

  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

  // ts_s time_ts1, time_ts2, time_ts3, time_ts4 = 0;
  // time_ts1 = rdtscp();
#ifdef PARALLEL_CHECKPOINT
  LOG_checkpoint_backward_parallel_apply(number_of_checkpoint_threads, starts, ends, pos, target_ts);
#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[2] += time_tmp;
#endif
#else
  writes_map.reserve(size_hashmap);

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[0] += time_tmp;

  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

  do {
    // time_ts3 = rdtscp();
    next_log = max_next_log(pos, starts, ends);
    i = next_log;
    if (next_log == -1) {
      break;
    }

    log = NH_global_logs[next_log];

    target_ts = max_tx_after(log, starts[next_log], target_ts, &(pos[next_log])); // updates the ptr
    // time_ts4 += rdtscp() - time_ts3;
    // advances to the next write after the TS

    // pos[next_log] must be between start and end
    if (pos[next_log] != starts[next_log]) {
      pos[next_log] = ptr_mod_log(pos[next_log], -1);
    } else {
      // ended
      break;
    }

    // within the start/end boundary
    assert((starts[i] <= ends[i] && pos[i] >= starts[i] &&
      pos[i] <= ends[i]) || (starts[i] > ends[i] &&
        ((pos[i] >= 0 && pos[i] < starts[i]) ||
        (pos[i] < LOG_local_state.size_of_log && pos[i] > ends[i]))
      )
    );

    NVLogEntry_s entry = log->ptr[pos[next_log]];
    ts_s ts = entry_is_ts(entry);
    while (!ts && pos[next_log] != starts[next_log]) {

#ifdef STAT
#ifdef WRITE_AMOUNT_NVHTM
        no_filter_write_amount += sizeof(entry.value);
#endif
#endif
#ifdef USE_PMEM
        // if ((uintptr_t)entry.addr < (uintptr_t)pmem_pool || (uintptr_t)((char *)pmem_pool + pmem_size) < (uintptr_t)entry.addr) {
        //     fprintf(stderr, "out of range\n");
        //     continue;
        // }
#endif
      // uses only the bits needed to identify the cache line
      intptr_t cl_addr = (((intptr_t)entry.addr >> 6) << 6);
      auto it = writes_map.find((GRANULE_TYPE*)cl_addr);
      int val_idx = ((intptr_t)entry.addr & 0x38) >> 3; // use bits 4,5,6
      char bit_map = 1 << val_idx;
      if (it == writes_map.end()) {
        // not found the write --> insert it
        CL_BLOCK block;
        block.bit_map = bit_map;
        /*block.block[val_idx] = entry.value;*/
        auto to_insert = make_pair((GRANULE_TYPE*)cl_addr, block);
        writes_map.insert(to_insert);
        writes_list.push_back((GRANULE_TYPE*)cl_addr);
        MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
#ifdef STAT
#ifdef WRITE_AMOUNT_NVHTM
        filtered_write_amount += sizeof(entry.value);
#endif
#endif
      } else {
        if ( !(it->second.bit_map & bit_map) ) {
          // Need to write this word
          MN_write(entry.addr, &(entry.value), sizeof(GRANULE_TYPE), 1);
          it->second.bit_map |= bit_map;
#ifdef STAT
#ifdef WRITE_AMOUNT_NVHTM
          filtered_write_amount += sizeof(entry.value);
#endif
#endif
#ifdef FAW_CHECKPOINT
          if (it->second.bit_map == -1) {
              MN_flush(it->first, CACHE_LINE_SIZE, 1);
          }
#endif
        }
      }

      pos[next_log] = ptr_mod_log(pos[next_log], -1);
      entry = log->ptr[pos[next_log]];
      ts = entry_is_ts(entry);
    }
    // NH_nb_applied_txs++;
  } while (next_log != -1);

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[2] += time_tmp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif

  // flushes the changes
  auto cl_iterator = writes_list.begin();
  // auto cl_it_end = writes_list.end();
  for (; cl_iterator != writes_list.end(); ++cl_iterator) {
    // TODO: must write the cache line, now is just spinning
    GRANULE_TYPE *addr = *cl_iterator;
#ifdef USE_PMEM
    MN_flush(addr, CACHE_LINE_SIZE, 1);
#else
    MN_flush(addr, CACHE_LINE_SIZE, 0);
#endif
  }
#ifdef USE_PMEM
  _mm_sfence();
#endif

#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
  time_tmp = 0;
  time_tmp += (edt.tv_nsec - stt.tv_nsec);
  time_tmp /= 1000000000;
  time_tmp += edt.tv_sec - stt.tv_sec;
  checkpoint_section_time[3] += time_tmp;
#endif
#endif

  // advance the pointers
  //    int freed_space = 0;
  for (i = 0; i < TM_nb_threads; ++i) {
    log = NH_global_logs[i];
    //        freed_space += distance_ptr(log->start, pos_to_start[i]);
    assert(starts[i] == log->start); // only this thread changes this
    // either in the boundary or just cleared the log
    assert((distance_ptr(starts[i], ends[i]) >=
    distance_ptr(pos_to_start[i], ends[i]))
    || (ptr_mod_log(ends[i], 1) == pos_to_start[i] ));
    //            printf("s:%i t:%i e:%i d1:%i d2:%i \n", starts[i], pos_to_start[i], ends[i],
    //                   distance_ptr(starts[i], ends[i]), distance_ptr(pos_to_start[i], ends[i]));

#ifdef USE_PMEM
    MN_write(&(log->start), &(pos_to_start[i]), sizeof(int), 1);
#else
    MN_write(&(log->start), &(pos_to_start[i]), sizeof(int), 0);
#endif
    // log->start = pos_to_start[i];
    // TODO: snapshot the old ptrs before moving them
  }
  *NH_checkpointer_state = 0;
  __sync_synchronize();
  return 0;
}
