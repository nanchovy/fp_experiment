#ifdef CONCURRENT
#  error "CONCURRENT is defined!"
#endif
#include "thread_manager.h"

void *bptreeThreadFunctionWrapper(void *container_v) {
    return 0;
}

void bptreeThreadInit(unsigned int flag) { }

void bptreeThreadDestroy() { }

pthread_t bptreeCreateThread(BPTree *bpt, void *(* thread_function)(BPTree *, void *), void *arg) {
    return 0;
}

void bptreeStartThread(void) { }

void bptreeWaitThread(pthread_t tid, void **retval) { }
