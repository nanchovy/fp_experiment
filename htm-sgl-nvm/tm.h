#ifndef TM_H
#define TM_H 1

#  include <stdio.h>
#  include <stdint.h>
#  include "timer.h"
#  include "nh.h"

#ifndef LOG_NAME
#  define LOG_NAME "./log"
#endif
#ifndef DATA_NAME
#  define DATA_NAME "./data"
#endif
#ifndef CP_THRNUM
#  define CP_THRNUM 8
#endif

#ifndef REDUCED_TM_API

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

// before start
#ifdef USE_PMEM
#  define GOTO_SIM()                    {INIT_ALLOCATOR();NVHTM_start_stats();}
#else
#  define GOTO_SIM()                    NVHTM_start_stats()
#endif
// after end
#  define GOTO_REAL()                   NVHTM_end_stats()
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#    define TM_BEGIN_NO_LOG()
#    define TM_END_NO_LOG()

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

#  include <assert.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#  include "thread.h"
#  include <math.h>

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */
#  define TM_CALLABLE                   /* nothing */

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

// do not track private memory allocation
#ifdef USE_PMEM
#  define INIT_SIZE (4 * 1073741824l)
extern void *stamp_mapped_file_pointer_g;
extern __thread void *stamp_mapped_file_pointer;
extern int stamp_number_of_thread_counter;
extern unsigned long stamp_length_of_mapped_area_per_thread;
extern __thread stamp_used_bytes;
extern __thread pthread_mutex_t tm_malloc_mutex;
#  define INIT_ALLOCATOR() {\
    if (stamp_mapped_file_pointer_g == NULL) {\
        pthread_mutex_lock(&tm_malloc_mutex);\
        if (stamp_mapped_file_pointer_g == NULL) {\
            stamp_mapped_file_pointer_g = NH_alloc(DATA_NAME, INIT_SIZE);\
            NVHTM_cpy_to_checkpoint(stamp_mapped_file_pointer_g);\
        }\
        pthread_mutex_unlock(&tm_malloc_mutex);\
        stamp_mapped_file_pointer = stamp_mapped_file_pointer_g;\
    }\
}
#  define ALLOCATOR_SET_SIZE_THR(threadnum) {\
    stamp_length_of_mapped_area_per_thread = (INIT_SIZE) / (threadnum);\
}
#  define INIT_ALLOCATOR_THR() {\
    int tid = __sync_fetch_and_add(&stamp_number_of_thread_counter, 1);\
    stamp_mapped_file_pointer = (void *)((char *)stamp_mapped_file_pointer_g + tid * stamp_length_of_mapped_area_per_thread);\
}
#  define EXIT_ALLOCATOR_THR() {\
    __sync_fetch_and_sub(&stamp_number_of_thread_counter, 1);\
}
#  define P_MALLOC(size)                ({\
    void *p = (void *)((char *)stamp_mapped_file_pointer + stamp_used_bytes);\
    stamp_used_bytes += size;\
    assert(stamp_used_bytes < stamp_length_of_mapped_area_per_thread);\
    /*assert(stamp_mapped_file_pointer_g <= p && p < (void *)((char *)stamp_mapped_file_pointer_g + INIT_SIZE));*/\
    assert(stamp_mapped_file_pointer <= p && p < (void *)((char *)stamp_mapped_file_pointer + stamp_length_of_mapped_area_per_thread));\
    p;\
})
/* NVHTM_alloc("alloc.dat", size, 0) */
#  define P_FREE(ptr)                   
#else
#  define INIT_ALLOCATOR()
#  define ALLOCATOR_SET_SIZE_THR(threadnum)
#  define P_MALLOC(size)                NH_alloc(size) /* NVHTM_alloc("alloc.dat", size, 0) */
#  define P_FREE(ptr)                   NH_free(ptr)
#endif

// TODO: free(ptr)
// the patched benchmarks are broken -> intruder gives me double free or corruption

// NVHTM_alloc("alloc.dat", size, 0)
#ifdef USE_PMEM
// #  define TM_MALLOC(size)                ({\
    if (stamp_mapped_file_pointer == NULL) {\
        stamp_mapped_file_pointer = NH_alloc(DATA_NAME, INIT_SIZE);\
        NVHTM_cpy_to_checkpoint(stamp_mapped_file_pointer);\
    }\
    void *p = (void *)((char *)stamp_mapped_file_pointer + stamp_used_bytes);\
    stamp_used_bytes += (size_t)(size + 0.9);\
    p;\
})
#  define TM_MALLOC(size)                ({\
    void *p = (void *)((char *)stamp_mapped_file_pointer + stamp_used_bytes);\
    stamp_used_bytes += size;\
    assert(stamp_used_bytes < stamp_length_of_mapped_area_per_thread);\
    assert(stamp_mapped_file_pointer <= p && p < (void *)((char *)stamp_mapped_file_pointer + stamp_length_of_mapped_area_per_thread));\
    /*assert(stamp_mapped_file_pointer_g <= p && p < (void *)((char *)stamp_mapped_file_pointer_g + INIT_SIZE));*/\
    p;\
})
/* NVHTM_alloc("alloc.dat", size, 0) */
#  define TM_FREE(ptr)           		    
#else
#  define TM_MALLOC(size)               NH_alloc(size) /* NVHTM_alloc("alloc.dat", size, 0) */
#  define TM_FREE(ptr)           		    NH_free(ptr)
#endif
#  define FAST_PATH_FREE(ptr)           TM_FREE(ptr)
#  define SLOW_PATH_FREE(ptr)           FAST_PATH_FREE(ptr)

# define SETUP_NUMBER_TASKS(n)
# define SETUP_NUMBER_THREADS(n) ALLOCATOR_SET_SIZE_THR(n+1)
# define PRINT_STATS()
# define AL_LOCK(idx)

extern __thread CL_ALIGN int GLOBAL_instrument_write;

#endif /* REDUCED_TM_API */

#ifdef REDUCED_TM_API
#    define SPECIAL_THREAD_ID()         get_tid()
#else /* REDUCED_TM_API */
#    define SPECIAL_THREAD_ID()         thread_getId()
#endif /* REDUCED_TM_API */

#ifndef USE_P8
#  include <immintrin.h>
#  include <rtmintrin.h>
#else /* USE_P8 */
#  include <htmxlintrin.h>
#endif /* USE_P8 */

// TODO:

#ifdef USE_PMEM
#  ifdef PARALLEL_CHECKPOINT
#    define TM_STARTUP(numThread)  NVHTM_set_cp_thread_num(CP_THRNUM); set_log_file_name(LOG_NAME); NVHTM_init(numThread); fprintf(stderr, "Budget=%i\n", HTM_SGL_INIT_BUDGET); HTM_set_budget(HTM_SGL_INIT_BUDGET); INIT_ALLOCATOR();  ALLOCATOR_SET_SIZE_THR(numThread+1)
#  else
#    define TM_STARTUP(numThread) set_log_file_name(LOG_NAME); NVHTM_init(numThread); fprintf(stderr, "Budget=%i\n", HTM_SGL_INIT_BUDGET); HTM_set_budget(HTM_SGL_INIT_BUDGET); INIT_ALLOCATOR();  ALLOCATOR_SET_SIZE_THR(numThread+1)
#  endif
#else
#  define TM_STARTUP(numThread)   NVHTM_init(numThread); printf("Budget=%i\n", HTM_SGL_INIT_BUDGET); HTM_set_budget(HTM_SGL_INIT_BUDGET)
#endif
#  define TM_SHUTDOWN()                NVHTM_shutdown()

#ifdef USE_PMEM
#  define TM_THREAD_ENTER()            NVHTM_thr_init(); HTM_set_budget(HTM_SGL_INIT_BUDGET); INIT_ALLOCATOR_THR()
#  define TM_THREAD_EXIT()             NVHTM_thr_exit(); EXIT_ALLOCATOR_THR()
#else
#  define TM_THREAD_ENTER()            NVHTM_thr_init(); HTM_set_budget(HTM_SGL_INIT_BUDGET)
#  define TM_THREAD_EXIT()             NVHTM_thr_exit()
#endif

// leave local_exec_mode = 0 to use the FAST_PATH
# define TM_BEGIN(b)                   NH_begin()
// # define TM_BEGIN_spec(b)              NH_begin_spec()
# define TM_BEGIN_ID(b)                NH_begin()

# define TM_BEGIN_EXT(b,a)             TM_BEGIN(b)
// # define TM_BEGIN_EXT_spec(b,a)        TM_BEGIN_spec(b)

# define TM_END()                      NH_commit()

#    define TM_BEGIN_RO()              TM_BEGIN(0)
#    define TM_RESTART()               NVHTM_abort_tx()
#    define TM_EARLY_RELEASE(var)

// TODO: check incompatible types
# define TM_RESTART()          NVHTM_abort_tx()
# define TM_SHARED_READ(var)   ({ NH_read(&(var)); })
# define TM_SHARED_READ_P(var) ({ NH_read_P(&(var)); })
# define TM_SHARED_READ_F(var) ({ NH_read_D(&(var)); })
# define TM_SHARED_READ_D(var) ({ NH_read_D(&(var)); })

#ifdef USE_PMEM
# define TM_SHARED_WRITE(var, val)   ({\
    if(GLOBAL_instrument_write) {\
        if (stamp_mapped_file_pointer_g <= &(var) && &(var) < (char *)stamp_mapped_file_pointer_g + INIT_SIZE) {\
            NH_write(&(var), val);\
        } else {\
            TM_LOCAL_WRITE(var, val);\
        }\
    } else {\
        TM_LOCAL_WRITE(var, val);\
    }\
    var;})
# define TM_SHARED_WRITE_P(var, val) ({\
    if(GLOBAL_instrument_write) {\
        if (stamp_mapped_file_pointer_g <= &(var) && &(var) < (char *)stamp_mapped_file_pointer_g + INIT_SIZE) {\
            NH_write_P(&(var), val);\
        } else {\
            TM_LOCAL_WRITE_P(var, val);\
        }\
    } else {\
        TM_LOCAL_WRITE_P(var, val);\
    }\
    var;})
# define TM_SHARED_WRITE_F(var, val) ({\
    if(GLOBAL_instrument_write) {\
        if (stamp_mapped_file_pointer_g <= &(var) && &(var) < (char *)stamp_mapped_file_pointer_g + INIT_SIZE) {\
            NH_write_D(&(var), val);\
        } else {\
            TM_LOCAL_WRITE_F(var, val);\
        }\
    } else {\
        TM_LOCAL_WRITE_F(var, val);\
    }\
    var;})
# define TM_SHARED_WRITE_D(var, val) ({\
    if(GLOBAL_instrument_write) {\
        if (stamp_mapped_file_pointer_g <= &(var) && &(var) < (char *)stamp_mapped_file_pointer_g + INIT_SIZE) {\
            NH_write_D(&(var), val);\
        } else {\
            TM_LOCAL_WRITE_D(var, val);\
        }\
    } else {\
        TM_LOCAL_WRITE_D(var, val);\
    }\
    var;})
#else
# define TM_SHARED_WRITE(var, val)   ({ if(GLOBAL_instrument_write) NH_write(&(var), val); else TM_LOCAL_WRITE(var, val); var;})
# define TM_SHARED_WRITE_P(var, val) ({ if(GLOBAL_instrument_write) NH_write_P(&(var), val); else TM_LOCAL_WRITE_P(var, val); var;})
# define TM_SHARED_WRITE_F(var, val) ({ if(GLOBAL_instrument_write) NH_write_D(&(var), val); else TM_LOCAL_WRITE_F(var, val); var;})
# define TM_SHARED_WRITE_D(var, val) ({ if(GLOBAL_instrument_write) NH_write_D(&(var), val); else TM_LOCAL_WRITE_D(var, val); var;})
#endif

# define FAST_PATH_SHARED_WRITE(var, val)   TM_SHARED_WRITE(var, val)
# define FAST_PATH_SHARED_WRITE_P(var, val) TM_SHARED_WRITE_P(var, val)
# define FAST_PATH_SHARED_WRITE_F(var, val) TM_SHARED_WRITE_F(var, val)
# define FAST_PATH_SHARED_WRITE_D(var, val) TM_SHARED_WRITE_D(var, val)

# define FAST_PATH_RESTART()          TM_RESTART()
# define FAST_PATH_SHARED_READ(var)   TM_SHARED_READ(var)
# define FAST_PATH_SHARED_READ_P(var) TM_SHARED_READ_P(var)
# define FAST_PATH_SHARED_READ_F(var) TM_SHARED_READ_F(var)
# define FAST_PATH_SHARED_READ_D(var) TM_SHARED_READ_D(var)

// not needed
# define SLOW_PATH_RESTART()                  FAST_PATH_RESTART()
# define SLOW_PATH_SHARED_READ(var)           FAST_PATH_SHARED_READ(var)
# define SLOW_PATH_SHARED_READ_P(var)         FAST_PATH_SHARED_READ_P(var)
# define SLOW_PATH_SHARED_READ_F(var)         FAST_PATH_SHARED_READ_D(var)
# define SLOW_PATH_SHARED_READ_D(var)         FAST_PATH_SHARED_READ_D(var)

# define SLOW_PATH_SHARED_WRITE(var, val)     FAST_PATH_SHARED_WRITE(var, val)
# define SLOW_PATH_SHARED_WRITE_P(var, val)   FAST_PATH_SHARED_WRITE_P(var, val)
# define SLOW_PATH_SHARED_WRITE_F(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)
# define SLOW_PATH_SHARED_WRITE_D(var, val)   FAST_PATH_SHARED_WRITE_D(var, val)

// local is not tracked
#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

#endif
