#ifndef NH_SOL_H
#define NH_SOL_H

#ifdef __cplusplus
extern "C"
{
  #endif
#ifdef USE_PMEM
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

  #define MAXIMUM_OFFSET 400 // in cycles

// TODO: remove the externs
  #undef BEFORE_HTM_BEGIN_spec
  #define BEFORE_HTM_BEGIN_spec(tid, budget) \
    extern __thread int global_threadId_; \
    extern int nb_transfers; \
    extern long nb_of_done_transactions; \
    while (*NH_checkpointer_state == 1 && nb_of_done_transactions < nb_transfers) { \
      PAUSE(); \
    }

  #undef BEFORE_TRANSACTION_i
  #define BEFORE_TRANSACTION_i(tid, budget) \
  LOG_get_ts_before_tx(tid); \
  LOG_before_TX(); \
  TM_inc_local_counter(tid);\

#ifdef STAT
  #undef BEFORE_HTM_BEGIN
  #define BEFORE_HTM_BEGIN(tid, budget) \
      clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_start);
#endif

  #undef BEFORE_COMMIT
  #define BEFORE_COMMIT(tid, budget, status) \
  ts_var = rdtscp(); /* must be the p version */  \
  if (LOG_count_writes(tid) > 0 && TM_nb_threads > 28) { \
    while ((rdtscp() - ts_var) < MAXIMUM_OFFSET); /* wait offset */ \
  }

#ifdef STAT
  #undef AFTER_TRANSACTION_i
  #define AFTER_TRANSACTION_i(tid, budget) ({ \
    int nb_writes = LOG_count_writes(tid); \
    if (nb_writes) { \
      htm_tx_val_counters[tid].global_counter = ts_var; \
      __sync_synchronize(); \
      NVMHTM_commit(TM_tid_var, ts_var, nb_writes); \
    } \
    CHECK_AND_REQUEST(tid); \
    TM_inc_local_counter(tid); \
    LOG_after_TX(); \
  })
#else
  #undef AFTER_TRANSACTION_i
  #define AFTER_TRANSACTION_i(tid, budget) ({ \
    int nb_writes = LOG_count_writes(tid); \
    if (nb_writes) { \
      htm_tx_val_counters[tid].global_counter = ts_var; \
      __sync_synchronize(); \
      NVMHTM_commit(TM_tid_var, ts_var, nb_writes); \
    } \
    CHECK_AND_REQUEST(tid); \
    LOG_after_TX(); \
  })
#endif

#ifdef STAT
  #undef AFTER_ABORT
  #define AFTER_ABORT(tid, budget, status) \
    clock_gettime(CLOCK_MONOTONIC_RAW, &transaction_abort_end);\
    double time_tmp = 0;                      \
    time_tmp += (transaction_abort_end.tv_nsec - transaction_start.tv_nsec);  \
    time_tmp /= 1000000000;                   \
    time_tmp += transaction_abort_end.tv_sec - transaction_start.tv_sec;      \
    abort_time_thread += time_tmp;  \
  /* NH_tx_time += rdtscp() - TM_ts1; */ \
  CHECK_LOG_ABORT(tid, status); \
  LOG_get_ts_before_tx(tid); \
  __sync_synchronize(); \
  ts_var = rdtscp(); \
  htm_tx_val_counters[tid].global_counter = ts_var; \
  /*CHECK_LOG_ABORT(tid, status);*/ \
  /*if (status == _XABORT_CONFLICT) printf("CONFLICT: [start=%i, end=%i]\n", \
  NH_global_logs[TM_tid_var]->start, NH_global_logs[TM_tid_var]->end); */
#else
  #undef AFTER_ABORT
  #define AFTER_ABORT(tid, budget, status) \
  CHECK_LOG_ABORT(tid, status); \
  LOG_get_ts_before_tx(tid); \
  __sync_synchronize(); \
  ts_var = rdtscp(); \
  htm_tx_val_counters[tid].global_counter = ts_var;
#endif

  #undef NH_before_write
  #define NH_before_write(addr, val) ({ \
    LOG_nb_writes++; \
    LOG_push_addr(TM_tid_var, addr, val); \
  })

#undef NH_write
#define NH_write(addr, val) ({ \
  GRANULE_TYPE buf = val; \
  NH_before_write(addr, val); \
  memcpy(addr, &(buf), sizeof(GRANULE_TYPE)); /* *((GRANULE_TYPE*)addr) = val; */ \
  NH_after_write(addr, val); \
  val; \
})

#ifdef USE_PMEM
#undef NH_alloc
#define NH_alloc(fn, size) ({ \
        pmem_filename = fn; \
        pmem_size = size; \
        pmem_pool = ALLOC_MEM(fn, size); \
        pmem_pool ;\
})
extern size_t pmem_size;
extern char const *pmem_filename;
extern void *pmem_pool;

#undef NH_free
#define NH_free(pool) FREE_MEM(pool, pmem_size)

#define REMAP_PRIVATE() {\
    int fd = open(pmem_filename, O_RDWR);\
    if (fd == -1) perror("open");\
    void *tmp = mmap(pmem_pool, pmem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fd, 0);\
    if (tmp == MAP_FAILED) perror("mmap");\
    close(fd);\
}
#endif


  // TODO: comment for testing with STAMP
  /* #ifndef USE_MALLOC
  #if DO_CHECKPOINT == 5
  #undef  NH_alloc
  #undef  NH_free
  #define NH_alloc(size) malloc(size)
  #define NH_free(pool)  free(pool)
  #else
  #undef  NH_alloc
  #undef  NH_free
  #define NH_alloc(size) NVHTM_malloc(size)
  #define NH_free(pool)  NVHTM_free(pool)
  #endif
  #endif */

  #ifdef __cplusplus
}
#endif

#endif /* NH_SOL_H */
