#ifndef CONCURRENT
#  error "CONCURRENT is not defined!"
#endif
#include "tree.h"
#include "thread_manager.h"
#include <stdlib.h>
#include <time.h>

int loop_times = 40;
int max_val = 1000;
void *update_threadfunc(BPTree *bpt, void *arg) {
    NVHTM_thr_init();
    unsigned char tid = *(unsigned char*)arg;
    unsigned int seed = tid;
    KeyValuePair kv;
    kv.value = 2;
    for (int i = 1; i <= loop_times; i++) {
        kv.key = rand_r(&seed) % max_val + 1;
        printf("update %ld\n", kv.key);
        if (bptreeUpdate(bpt, kv, tid)) {
            printf("success\n");
        } else {
            printf("failure\n");
        }
    }
    NVHTM_thr_exit();
}

void *warmup(BPTree * bpt, void *args) {
}

int main(int argc, char *argv[]) {
    BPTree *bpt;
    KeyValuePair kv;
    int thread_num = 4;
    pthread_t *tid_array = NULL;
    if (argc > 1) {
        loop_times = atoi(argv[1]);
        if (loop_times <= 0) {
            printf("invalid argument\n");
            return 1;
        }
        max_val = atoi(argv[2]);
        if (max_val <= 0) {
            printf("invalid argument\n");
            return 1;
        }
    } else {
        printf("default: loop_times = 40, max_val = 1000\n");
    }
    size_t allocation_size = sizeof(LeafNode) * loop_times * thread_num / (MAX_KEY / 2);
#ifdef USE_PMEM
    set_log_file_name("log");
#endif
    NVHTM_init(thread_num + 1);
#ifdef USE_PMEM
    void *pool = NH_alloc("data", allocation_size);
#else
    void *pool = NH_alloc(allocation_size);
#endif
    printf("pool = %p\n", pool);
    NVHTM_thr_init();
    initAllocator(pool, "data", allocation_size, thread_num);
    NVHTM_clear();
    bpt = newBPTree();
    kv.key = 1;
    kv.value = 1;
    srand((unsigned) time(NULL));
    for (int i = 1; i <= loop_times * thread_num; i++) {
        kv.key = rand() % max_val;
        printf("insert %ld\n", kv.key);
        if (insert(bpt, kv, thread_num)) {
            printf("success\n");
        } else {
            printf("failure\n");
        }
    }

    tid_array = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    NVHTM_cpy_to_checkpoint(pool);
    bptreeThreadInit(BPTREE_NONBLOCK);

    unsigned char *t_arg = (unsigned char*)malloc(sizeof(unsigned char) * thread_num);
    for (int i = 0; i < thread_num; i++) {
        t_arg[i] = i + 1;
        tid_array[i] = bptreeCreateThread(bpt, update_threadfunc, warmup, t_arg + i);
    }

    for (int i = 0; i < thread_num; i++) {
        bptreeWaitThread(tid_array[i], NULL);
    }
    free(t_arg);
    free(tid_array);

    bptreeThreadDestroy();

    showTree(bpt, 0);
    destroyAllocator();

    NVHTM_thr_exit();
    NVHTM_shutdown();

    return 0;
}
