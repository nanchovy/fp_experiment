#ifndef CONCURRENT
#  error "CONCURRENT is not defined!"
#endif
#include "thread_manager.h"

unsigned int number_of_thread = 0;
unsigned int cpu_counter = 0;
unsigned int waiting_thread = 0;
unsigned int start_threads = 0;

void set_affinity() {
    cpu_set_t mask;
    unsigned int assigned_cpu = __sync_fetch_and_add(&cpu_counter, 1);
    CPU_ZERO(&mask);
    CPU_SET(assigned_cpu, &mask);
    int err = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
    if (err == -1) {
        perror("sched_setaffinity:");
    }
}

unsigned int ready_threads() {
    return waiting_thread;
}

void *bptreeThreadFunctionWrapper(void *container_v) {
    BPTreeFunctionContainer *container = (BPTreeFunctionContainer *)container_v;
    set_affinity();
    __sync_fetch_and_add(&waiting_thread, 1);
    container->warmup(container->bpt, container->arg);
    while (start_threads == 0) {
        _mm_pause();
    }
    container->retval = container->function(container->bpt, container->arg);
    pthread_exit(container);
}

void bptreeThreadInit(unsigned int flag) {
    if (flag & BPTREE_BLOCK) {
        start_threads = 0;
    } else {
        start_threads = 1;
    }
}

void bptreeThreadDestroy() {
}

pthread_t bptreeCreateThread(BPTree *bpt, void *(* thread_function)(BPTree *, void *), void *(* warmup_function)(BPTree *, void *), void *arg) {
    pthread_t tid;
    BPTreeFunctionContainer *container = (BPTreeFunctionContainer *)vol_mem_allocate(sizeof(BPTreeFunctionContainer));
    container->function = thread_function;
    container->bpt = bpt;
    container->retval = NULL;
    container->arg = arg;
    container->warmup = warmup_function;
    number_of_thread++;
    if (pthread_create(&tid, NULL, bptreeThreadFunctionWrapper, container) == EAGAIN) {
        printf("pthread_create: reached resource limit\n");
    }
    return tid;
}

void bptreeStartThread(void) {
    start_threads = 1;
}

void bptreeWaitThread(pthread_t tid, void **retval) {
    void *container_v;
    pthread_join(tid, &container_v);
    if (retval != NULL && container_v != NULL) {
        *retval = ((BPTreeFunctionContainer *)container_v)->retval;
    }
    vol_mem_free(container_v);
    number_of_thread--;
}
