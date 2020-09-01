#ifndef EXTRA_GLOBALS_H
#define EXTRA_GLOBALS_H

#include "extra_types.h"

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
extern __thread struct timespec transaction_start;
extern __thread struct timespec transaction_abort_end;
extern unsigned int *checkpoint_by_flags;
extern unsigned int checkpoint_by[3];
extern double checkpoint_section_time[4];
extern int *checkpoint_empty;
#ifdef WRITE_AMOUNT_NVHTM
extern unsigned long no_filter_write_amount;
#endif
#endif

#endif /* EXTRA_GLOBALS_H */
