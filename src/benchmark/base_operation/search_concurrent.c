#include "tree.h"
#include "random.h"
#include "thread_manager.h"
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#define WARMUP_NUM 50

int initial_elements = 40;
int loop_times = 40;
int max_val = 1000;
int thread_max = 10;
int cp_threads = 1;

typedef struct arg_t {
	unsigned int loop;
	unsigned int seed;
	unsigned char tid;
} arg_t;

void *warmup(BPTree *bpt, void *args) {
    LeafNode *next = bpt->head;
    KeyValuePair dummy;
    while (next != NULL) {
#ifdef FPTREE
        dummy = next->pleaf->kv[MAX_PAIR/2];
#else
        dummy = next->kv[next->key_length/2];
#endif
        next = next->next;
    }
    // _mm_prefetch(bpt->root, _MM_HINT_T0);
    return NULL;
}

void *insert_random(BPTree *bpt, void *arg) {
#ifdef NVHTM
    NVHTM_thr_init();
#endif
    arg_t *arg_cast = (arg_t *)arg;
    unsigned char tid = arg_cast->tid;
    KeyValuePair kv;
    unsigned int seed = arg_cast->seed;
    unsigned int loop = arg_cast->loop;
    kv.key = 1;
    kv.value = 1;
    for (int i = 0; i < loop; i++) {
        kv.key = get_rand(i, tid-1) % INT_MAX + 1;
        // printf("inserting %ld\n", kv.key);
        if (!insert(bpt, kv, tid)) {
            // fprintf(stderr, "insert: failure\n");
        }
        // showTree(bpt);
    }
    show_result_thread(tid);
#ifdef NVHTM
    NVHTM_thr_exit();
#endif
    return arg;
}

void *search_random(BPTree *bpt, void *arg) {
    arg_t *arg_cast = (arg_t *)arg;
#ifdef NVHTM
    NVHTM_thr_init();
#endif
    unsigned char tid = arg_cast->tid;
    SearchResult sr;
    Key key = 1;
    unsigned int seed = arg_cast->seed;
    unsigned int loop = arg_cast->loop;
    for (int i = 0; i < loop; i++) {
        key = get_rand(i, tid-1) % INT_MAX + 1;
        search(bpt, key, &sr, tid);
    }
    show_result_thread(tid);
#ifdef NVHTM
    NVHTM_thr_exit();
#endif
    return NULL;
}

char const *pmem_path = "data";
char const *log_path = "log";

int main(int argc, char *argv[]) {
    pthread_t *tid_array;
    arg_t **arg;
    struct timespec stt, edt;
    int i;
    BPTree *bpt;
    KeyValuePair kv;
    if (argc > 4) {
        initial_elements = atoi(argv[1]);
        if (initial_elements <= 0) {
            fprintf(stderr, "invalid argument\n");
            return 1;
        }
        loop_times = atoi(argv[2]);
        if (loop_times <= 0) {
            fprintf(stderr, "invalid argument\n");
            return 1;
        }
        max_val = atoi(argv[3]);
        if (max_val <= 0) {
            fprintf(stderr, "invalid argument\n");
            return 1;
        }
        thread_max = atoi(argv[4]);
        if (thread_max <= 0) {
            fprintf(stderr, "invalid argument\n");
            return 1;
        }
        pmem_path = argv[5];
        log_path = argv[6];
#ifdef PARALLEL_CHECKPOINT
        if (argc > 7) {
            cp_threads = atoi(argv[7]);
            if (cp_threads <= 0) {
                fprintf(stderr, "invalid argument\n");
                return 1;
            }
            fprintf(stderr, "cp_threads = %d\n", cp_threads);
        }
#endif
        fprintf(stderr, "initial_elements = %d, loop_times = %d, max_val = %d, thread_max = %d, pmem_path = %s, log_path = %s\n", initial_elements, loop_times, max_val, thread_max, pmem_path, log_path);
    } else {
        fprintf(stderr, "default: initial_elements = %d, loop_times = %d, max_val = %d, thread_max = %d, pmem_path = %s, log_path = %s\n", initial_elements, loop_times, max_val, thread_max, pmem_path, log_path);
    }

#ifdef BPTREE
    size_t allocation_size = sizeof(LeafNode) * (initial_elements / (MAX_PAIR/2) + 2 + thread_max) + sizeof(AllocatorHeader);
#else
    size_t allocation_size = sizeof(PersistentLeafNode) * (initial_elements / (MAX_PAIR/2) + 2 + thread_max) + sizeof(AllocatorHeader);
#endif
    fprintf(stderr, "allocating %lu byte\n", allocation_size);
#ifdef NVHTM
#ifdef PARALLEL_CHECKPOINT
    NVHTM_set_cp_thread_num(cp_threads);
#endif
#ifdef USE_PMEM
    set_log_file_name(log_path);
#endif
    NVHTM_init(thread_max + 2);
#ifdef USE_PMEM
    void *pool = NH_alloc(pmem_path, allocation_size);
#else
    void *pool = NH_alloc(allocation_size);
#endif
    NVHTM_clear();
    NVHTM_cpy_to_checkpoint(pool);
    initAllocator(pool, pmem_path, allocation_size, thread_max + 1);
    NVHTM_thr_init();
#else
    initAllocator(NULL, pmem_path, allocation_size, thread_max + 1);
#endif
    random_init(initial_elements, 0, 0);
    bpt = newBPTree();

    tid_array = (pthread_t *)malloc(sizeof(pthread_t) * (thread_max + 1));
    arg = (arg_t **)malloc(sizeof(arg_t *) * (thread_max + 1));

    // bptreeThreadInit(BPTREE_NONBLOCK);

    kv.key = 1;
    kv.value = 1;
    unsigned int seed = thread_max;

    for (int i = 0; i < initial_elements; i++) {
        kv.key = get_rand_initials(i) % INT_MAX + 1;
        // printf("inserting %ld\n", kv.key);
        if (!insert(bpt, kv, thread_max)) {
            // fprintf(stderr, "insert: failure\n");
        }
        // showTree(bpt);
    }
    // show_result_thread(thread_max);
// #ifdef NVHTM
//     NVHTM_thr_exit();
// #endif
    // arg[thread_max] = (arg_t *)malloc(sizeof(arg_t));
    // arg[thread_max]->seed = thread_max;
    // arg[thread_max]->tid = thread_max;
    // arg[thread_max]->loop = initial_elements;
    // tid_array[thread_max] = bptreeCreateThread(bpt, insert_random, arg[thread_max]);
    // bptreeWaitThread(tid_array[thread_max], NULL);
    // free(arg[thread_max]);
    random_destroy();
    random_init(0, loop_times, thread_max);

    bptreeThreadInit(BPTREE_BLOCK);

    for (i = 0; i < thread_max-1; i++) {
        arg[i] = (arg_t *)malloc(sizeof(int));
        arg[i]->seed = i;
        arg[i]->tid = i % 256 + 1;
        arg[i]->loop = loop_times / thread_max;
        tid_array[i] = bptreeCreateThread(bpt, search_random, warmup, arg[i]);
    }
    arg[i] = (arg_t *)malloc(sizeof(int));
    arg[i]->seed = i;
    arg[i]->tid = i % 256 + 1;
    arg[i]->loop = loop_times / thread_max + loop_times % thread_max;
    tid_array[i] = bptreeCreateThread(bpt, search_random, warmup, arg[i]);

    while (ready_threads() < thread_max) {
        _mm_pause();
    }

#ifdef NVHTM
    wait_for_checkpoint();
    NH_reset();
#else
    FREQ_WRITE_START();
    RESET_COUNT_ABORT();
#endif
    clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
    bptreeStartThread();

    for (i = 0; i < thread_max; i++) {
        bptreeWaitThread(tid_array[i], NULL);
        free(NULL);
    }
#ifdef NVHTM
    wait_for_checkpoint();
#endif
    clock_gettime(CLOCK_MONOTONIC_RAW, &edt);

    fprintf(stderr, "finish running threads\n");

    double time = edt.tv_nsec - stt.tv_nsec;
    time /= 1000000000;
    time += edt.tv_sec - stt.tv_sec;
    printf("%lf\n", time);

    // showTree(bpt, 0);

    destroyBPTree(bpt, 1);

    bptreeThreadDestroy();

    destroyAllocator();
    random_destroy();

#ifdef NVHTM
    NH_thr_reset();
    NVHTM_thr_exit();
    NVHTM_shutdown();
#endif

    return 0;
}
