#include "nvhtm.h"
#include <pthread.h>

int thread_num = 1;
int tid = 0;
int loop_times = 30;
int write_times = 200;

void *simple_write(void *arg) {
    long tmp;
    long *p = (long *)arg;
    int tid = __sync_fetch_and_add(&tid, 1);
    NVHTM_thr_init();
    for (int i = 1; i <= loop_times; i++) {
        NH_begin();
        for (int j = 0; j < write_times; j++) {
            NH_write(p, i);
        }
        tmp = NH_read(p);
        NH_commit();
        printf("simple_write %d: tmp = %ld\n", tid, tmp);
    }
    NVHTM_thr_exit();
    return NULL;
}

int main(int argc, char *argv[]) {
    int i;
    pthread_t tid[thread_num];
    NVHTM_init(thread_num + 1);
    size_t pool_sz = sizeof(long);
#ifdef USE_PMEM
    void *pool = NH_alloc("data", pool_sz);
#else
    void *pool = NH_alloc(pool_sz);
#endif
    printf("pool:%p -> %p (%lu)\n", pool, (char *)pool + pool_sz, pool_sz);
    NVHTM_clear();
    NVHTM_cpy_to_checkpoint(pool);

    for (i = 0; i < thread_num; i++) {
        pthread_create(&tid[i], NULL, simple_write, pool);
    }

    for (i = 0; i < thread_num; i++) {
        pthread_join(tid[i], NULL);
    }

    NVHTM_thr_exit();
    NVHTM_shutdown();

    return 0;
}
