#include "extra_globals.h"

// ####################################################
// ### LOG VARIABLES ##################################
// ####################################################
// global

CL_ALIGN NVLog_s **NH_global_logs;
void* LOG_global_ptr;
int is_sigsegv = 0;
// thread local
__thread CL_ALIGN NVLog_s *nvm_htm_local_log;
__thread CL_ALIGN int LOG_nb_wraps;
__thread CL_ALIGN NVLogLocal_s LOG_local_state;
#ifdef STAT
__thread double abort_time_thread = 0;
double abort_time_all = 0;
__thread struct timespec transaction_start;
__thread struct timespec transaction_abort_end;
unsigned int *checkpoint_by_flags;
unsigned int checkpoint_by[3];
double checkpoint_section_time[4];
int *checkpoint_empty;
#ifdef WRITE_AMOUNT_NVHTM
unsigned long no_filter_write_amount;
#endif
#endif
// ####################################################
