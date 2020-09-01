#ifndef TREE_H
#define TREE_H
#  ifdef __cplusplus
extern "C" {
#  endif

#include <assert.h>
#include <pthread.h>

/* definition of structs */
/* value should be NULL and key must be 0 when pair is unused.
 * valid key should be larger than 0.
 */
typedef long Key;
typedef int Value;
#ifdef NVHTM
extern const Key UNUSED_KEY;
extern const Value INITIAL_VALUE;
#else
#  define UNUSED_KEY -1
#  define INITIAL_VALUE 0
#endif

#ifdef TRANSACTION_SIZE
extern __thread unsigned int transaction_counter;
extern __thread unsigned int transaction_counter_max;
#  define COUNTUP_TRANSACTION_SIZE() {\
        transaction_counter++;\
    }
#  define UPDATE_TRANSACTION_SIZE() {\
        if (transaction_counter > transaction_counter_max)\
            transaction_counter_max = transaction_counter;\
        transaction_counter = 0;\
    }
#  define SHOW_TRANSACTION_SIZE() {\
        fprintf(stderr, "max of transaction size = %u\n", transaction_counter_max);\
    }
#else
#  define COUNTUP_TRANSACTION_SIZE()
#  define UPDATE_TRANSACTION_SIZE()
#  define SHOW_TRANSACTION_SIZE()
#endif

#ifdef FREQ_WRITE
#  define FREQ_WRITE_BUFSZ 2 * 60 * 60 * 17
#  define FREQ_INTERVAL 1024 * 1024
extern unsigned int wrote_size_tmp;
extern char freq_write_buf[FREQ_WRITE_BUFSZ];
extern int freq_write_buf_index;
extern int freq_write_start;
#  define FREQ_WRITE_START() {\
    freq_write_start = 1;\
}

#  define FREQ_WRITE_LOG(time) {\
   if (freq_write_buf_index + 17 <= FREQ_WRITE_BUFSZ) {\
     sprintf(freq_write_buf + freq_write_buf_index, "w%15lf\n", time);\
     freq_write_buf_index += 17;\
   }\
}
#  define FREQ_WRITE_ADD(sz) {\
      if (freq_write_start) {\
          unsigned int tmp = __sync_fetch_and_add(&wrote_size_tmp, (sz));\
          if (tmp + (sz) > FREQ_INTERVAL) {\
              if (__sync_bool_compare_and_swap(&wrote_size_tmp, tmp + (sz), 0)) {\
                  struct timespec tm;\
                  double time_tmp = 0;\
                  clock_gettime(CLOCK_MONOTONIC_RAW, &tm);\
                  time_tmp += tm.tv_nsec;  \
                  time_tmp /= 1000000000;                   \
                  time_tmp += tm.tv_sec;      \
                  FREQ_WRITE_LOG(time_tmp);\
              }\
          }\
      }\
  }
#  define SHOW_FREQ_WRITE() {\
     FILE *ofile = fopen("write_freq.txt", "w");\
     if (ofile == NULL) {\
         perror("SHOW_FREQ_WRITE");\
     } else {\
         freq_write_buf[freq_write_buf_index] = '\0';\
         fprintf(ofile, "%s", freq_write_buf);/*fprintf(stderr, "%s", freq_write_buf);*/\
         fclose(ofile);\
     }\
    /*printf("wrote_size_tmp = %u\n", wrote_size_tmp);*/\
   }

#else
#  define FREQ_WRITE_START()
#  define FREQ_WRITE_ADD(sz)
#  define SHOW_FREQ_WRITE()
#endif

#ifdef COUNT_ABORT
extern __thread unsigned int times_of_lock;
extern __thread unsigned int times_of_transaction;
extern unsigned int times_of_lock_sum;
extern unsigned int times_of_transaction_sum;
extern __thread unsigned int times_of_abort[4];
extern unsigned int times_of_tree_abort[4];

#  define RESET_COUNT_ABORT() {\
    int i;\
    fprintf(stderr, "resetting abort counters:\n");\
    fprintf(stderr, "lock = %u\n", times_of_lock_sum);\
    fprintf(stderr, "transaction = %u\n", times_of_transaction_sum);\
    times_of_lock_sum = 0;\
    times_of_transaction_sum = 0;\
    for (i = 0; i < 4; i++) {\
        fprintf(stderr, "abort[%d] = %u\n", i, times_of_tree_abort[i]);\
        times_of_tree_abort[i] = 0;\
    }\
}
#  define TRANSACTION_SUCCESS() times_of_transaction++
#  define LOCK_SUCCESS() times_of_lock++
#  define ABORT_OCCURRED(type) {   \
     /*printf("abort -> %d\n", type);*/  \
    if (type & (0x1 << 0)) {        \
        times_of_abort[0]++;        \
    } else if (type & (0x1 << 2)) { \
        times_of_abort[1]++;        \
    } else if (type & (0x1 << 3)) { \
        times_of_abort[2]++;        \
    } else {                        \
        times_of_abort[3]++;        \
    }                               \
}
#  define SUM_COUNT_ABORT() {\
    __sync_fetch_and_add(&times_of_lock_sum, times_of_lock);\
    __sync_fetch_and_add(&times_of_transaction_sum, times_of_transaction);\
    __sync_fetch_and_add(&times_of_tree_abort[1], times_of_abort[1]);\
	__sync_fetch_and_add(&times_of_tree_abort[0], times_of_abort[0]);\
    __sync_fetch_and_add(&times_of_tree_abort[2], times_of_abort[2]);\
    __sync_fetch_and_add(&times_of_tree_abort[3], times_of_abort[3]);\
    fprintf(stderr, "abort[0] user     = %u times\n", times_of_abort[0]);\
    fprintf(stderr, "abort[1] conflict = %u times\n", times_of_abort[1]);\
    fprintf(stderr, "abort[2] capacity = %u times\n", times_of_abort[2]);\
    fprintf(stderr, "abort[3] other    = %u times\n", times_of_abort[3]);\
}
#  define SHOW_COUNT_ABORT() {\
	fprintf(stderr, "executed by lock = %u times\n", times_of_lock_sum);\
    fprintf(stderr, "executed by transaction = %u times\n", times_of_transaction_sum);\
    fprintf(stderr, "abort[0] user     = %u times\n", times_of_tree_abort[0]);\
    fprintf(stderr, "abort[1] conflict = %u times\n", times_of_tree_abort[1]);\
    fprintf(stderr, "abort[2] capacity = %u times\n", times_of_tree_abort[2]);\
    fprintf(stderr, "abort[3] other    = %u times\n", times_of_tree_abort[3]);\
}
#else
#  define SUM_COUNT_ABORT()
#  define RESET_COUNT_ABORT()
#  define TRANSACTION_SUCCESS()
#  define LOCK_SUCCESS()
#  define SHOW_COUNT_ABORT()
#  define ABORT_OCCURRED(type)
#endif

#ifdef COUNT_WRITE 
static unsigned long nvm_write_count = 0;

#  define WRITE_COUNT_UP() ({\
      __sync_fetch_and_add(&nvm_write_count, 1);\
   })
#  define GET_WRITE_COUNT() (nvm_write_count)
#  define SHOW_WRITE_COUNT() {\
    fprintf(stderr, "write count: %lu\n", GET_WRITE_COUNT());\
}
#else
#  define WRITE_COUNT_UP()
#  define GET_WRITE_COUNT() (0l)
#  define SHOW_WRITE_COUNT()
#endif

#ifdef WRITE_AMOUNT
extern __thread unsigned long write_amount_thr;
extern unsigned long write_amount;
#define WRITE_AMOUNT_REC(size) {\
    write_amount_thr += size;\
}
#define WRITE_AMOUNT_ADD() {\
    fprintf(stderr, "adding WA = %lu <- %lu\n", write_amount, write_amount_thr);\
    __sync_fetch_and_add(&write_amount, write_amount_thr);\
}
#define SHOW_WRITE_AMOUNT() {\
    fprintf(stderr, "write amount = %lu\n", write_amount);\
}
#else
#define WRITE_AMOUNT_REC(size)
#define WRITE_AMOUNT_ADD()
#define SHOW_WRITE_AMOUNT()
#endif

#define NVM_WRITE(p, v) ({\
      WRITE_COUNT_UP();\
      FREQ_WRITE_ADD(sizeof(v));\
      (*p = v);\
  })

/* structs */
typedef struct KeyValuePair {
    Key key;
    Value value;
} KeyValuePair;

#ifdef BPTREE
#  include "bptree.h"
#else
#  include "fptree.h"
#endif

void show_result_thread(unsigned char);

BPTree *newBPTree();
void destroyBPTree(BPTree *, unsigned char);
void search(BPTree *, Key, SearchResult *, unsigned char);

int insert(BPTree *, KeyValuePair, unsigned char);
int bptreeUpdate(BPTree *, KeyValuePair, unsigned char);
int bptreeRemove(BPTree *, Key, unsigned char);

/* debug function */
void showTree(BPTree *, unsigned char);

#  ifdef __cplusplus
};
#  endif
#endif
