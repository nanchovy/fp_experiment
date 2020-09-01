#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H
#  ifdef __cplusplus
extern "C" {
#  endif
#  define _GNU_SOURCE
#  include <sched.h>
#  include "tree.h"
#  ifdef CONCURRENT
#    include <pthread.h>
#    include <immintrin.h>
#    include <stdlib.h>
#    include <errno.h>
#    include <stdio.h>
struct BPTreeFunctionContainer {
    void *(* function)(BPTree *, void *);
    void *(* warmup)(BPTree *, void *);
    BPTree *bpt;
    void *retval;
    void *arg;
};
typedef struct BPTreeFunctionContainer BPTreeFunctionContainer;
#    endif

#    define BPTREE_BLOCK 1
#    define BPTREE_NONBLOCK 2

unsigned int ready_threads();
void bptreeThreadInit(unsigned int flag);
void bptreeThreadDestroy();
pthread_t bptreeCreateThread(BPTree *, void *(*)(BPTree *, void *), void *(*)(BPTree *, void *), void *);
void bptreeStartThread(void);
void bptreeWaitThread(pthread_t, void **);

#  ifdef __cplusplus
};
#  endif
#endif
