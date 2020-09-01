#ifndef NVHTM_WRAPPER_H
#define NVHTM_WRAPPER_H
#  ifdef NVHTM
#    include "nvhtm.h"
#    define NVHTM_begin() NH_begin()
#    define NVHTM_end() {\
       NH_commit();\
       UPDATE_TRANSACTION_SIZE();\
     }
#    define NVM_write(p, val) {\
       NH_write(p, val);\
       COUNTUP_TRANSACTION_SIZE();\
     }
#    define NVM_read(p) NH_read(p)
#  else
#    define NVHTM_begin()
#    define NVHTM_thr_init()
#    define NVHTM_thr_exit()
#    define NVHTM_init(d)
#    define NVHTM_clear()
#    define NVHTM_shutdown()
#    define NVHTM_cpy_to_checkpoint(p)
#    define NVHTM_end()
#    define NH_alloc(s) malloc(s)
#    define NVM_write(p, val) *p = val
#    define NVM_read(p) *p
#  endif
#endif
