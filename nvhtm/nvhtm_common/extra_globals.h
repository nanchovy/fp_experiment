#ifndef EXTRA_GLOBALS_H
#define EXTRA_GLOBALS_H

#include "extra_types.h"
#include <semaphore.h>

// ####################################################
// ### LOG VARIABLES ##################################
// ####################################################
// global
extern CL_ALIGN NVLog_s **NH_global_logs;
extern void* LOG_global_ptr;
// thread local
extern __thread CL_ALIGN NVLog_s *nvm_htm_local_log;
extern __thread CL_ALIGN int LOG_nb_wraps;
extern __thread CL_ALIGN NVLogLocal_s LOG_local_state;
// ####################################################
#ifdef STAT
extern __thread double abort_time_thread;
extern double abort_time_all;
extern __thread double transaction_time_thread;
extern double transaction_time_all;
extern __thread struct timespec transaction_start;
extern __thread struct timespec transaction_abort_end;
extern __thread struct timespec transaction_commit;
extern unsigned int *checkpoint_by_flags;
extern unsigned int checkpoint_by[3];
extern double checkpoint_section_time[4];
extern int *checkpoint_empty;
#ifdef WRITE_AMOUNT_NVHTM
extern unsigned long no_filter_write_amount;
extern unsigned long filtered_write_amount;
#endif
#endif
#ifdef PARALLEL_CHECKPOINT
  typedef struct checkpoint_args_s {
    // intptr_t mask;
    // intptr_t assigned_addr;
    // int assigned_addr;
    int thread_id;
    int number_of_threads;
  } checkpoint_args_s;
extern sem_t cp_back_sem;
extern int number_of_checkpoint_threads;
extern sem_t cpthread_finish_sem;
extern checkpoint_args_s *cp_thread_args;
#ifdef STAT
#  ifdef CHECK_TASK_DISTRIBUTION
extern unsigned int *applied_entries;
#  endif
#  ifdef NUMBER_OF_ENTRIES
extern unsigned int *read_entries;
extern unsigned int *wrote_entries;
#  endif
#    ifdef MEASURE_PART_CP
#      define CPTIME_NUM 4
#    else
#      define CPTIME_NUM 2
#    endif
extern double *parallel_checkpoint_section_time_thread[];
#endif
#ifdef LOG_COMPRESSION
extern NVLog_s **NH_global_compressed_logs;
extern void* LOG_compressed_global_ptr;
#endif
#endif

#endif /* EXTRA_GLOBALS_H */
