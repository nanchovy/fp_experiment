#include "tree.h"
#include "thread_manager.h"
#include <stdlib.h>
#include <time.h>

void *bit_test(BPTree *bpt, void *arg) {
#ifdef NVHTM
    NVHTM_thr_init();
    char *bit = (char *)arg;
    printf("bit = %x\n", *bit);
    NVHTM_begin();
    SET_BIT_T(bit, 0);
    NVHTM_end();
    printf("bit = %x\n", *bit);
    NVHTM_begin();
    CLR_BIT_T(bit, 0);
    NVHTM_end();
    printf("bit = %x\n", *bit);
    NVHTM_thr_exit();
#endif
    return NULL;
}

int main(int argc, char *argv[]) {
#ifdef NVHTM
    NVHTM_init(2);
    size_t pool_sz = 128;
#ifdef USE_PMEM
    void *pool = NH_alloc("data", pool_sz);
#else
    void *pool = NH_alloc(pool_sz);
#endif
    NVHTM_clear();
    NVHTM_cpy_to_checkpoint(pool);

    bptreeThreadInit(BPTREE_NONBLOCK);
    pthread_t tid = bptreeCreateThread(NULL, bit_test, pool);
    bptreeWaitThread(tid, NULL);

    NVHTM_shutdown();
#endif

    return 0;
}
