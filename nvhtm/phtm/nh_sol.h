#ifndef NH_SOL_H
#define NH_SOL_H

#include "utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#undef BEFORE_TRANSACTION_i
#define BEFORE_TRANSACTION_i(tid, budget) \
    __sync_synchronize(); \
	LOG_nb_writes = 0;

// flushes the log transparently
#undef BEFORE_COMMIT
#define BEFORE_COMMIT(tid, budget, status) ({ \
	SPIN_PER_WRITE(PHTM_log_size(tid)); \
})

#undef AFTER_TRANSACTION_i
#define AFTER_TRANSACTION_i(tid, budget) ({ \
	SPIN_PER_WRITE(PHTM_log_size(tid)); /* CLFLUSH the writes */ \
	PHTM_log_clear(); /* destroy the log */ \
})

// TODO: NVHTM_write also changes with the solution...
// move it here!
#undef NH_before_write
#define NH_before_write(addr, val) ({ \
	PHTM_instrument_write(TM_tid_var, addr, val); \
})

#undef NH_before_read
#define NH_before_read(addr) ({ \
	PHTM_instrument_read(TM_tid_var, addr); \
})

#ifdef __cplusplus
}
#endif

#endif /* NH_SOL_H */

