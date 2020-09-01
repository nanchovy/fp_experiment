#include "tree.h"
#include "thread_manager.h"
#include <stdlib.h>
#include <time.h>

#define SEED_INIT_VAL 0

int loop_times = 40;
int max_val = 1000;

void *insert_test(BPTree *bpt, void *arg) {
    unsigned char tid = *(unsigned char *) arg;
    KeyValuePair kv;
    unsigned int seed = SEED_INIT_VAL;
    kv.key = 1;
    kv.value = 1;
    for (int i = 1; i <= loop_times; i++) {
        kv.key = rand_r(&seed) % max_val + 1;
        printf("insert: target = %ld\n", kv.key);
        if (insert(bpt, kv, tid)) {
            printf("insert: success\n");
        } else {
            printf("insert: failure\n");
        }
    }
    printf("insert: tree state -----------------------------\n");
    showTree(bpt, tid);
    printf("------------------------------------------------\n");
    return NULL;
}

void *search_test(BPTree *bpt, void *arg) {
    unsigned char tid = *(unsigned char *) arg;
    Key key = 1;
    unsigned int seed = SEED_INIT_VAL;
    struct timespec wait_time;
    wait_time.tv_sec = 0;
    wait_time.tv_nsec = 10;
    SearchResult sr;
    for (int i = 1; i <= loop_times; i++) {
        key = rand() % max_val;
        printf("search: target = %ld\n", key);
        search(bpt, key, &sr, tid);
        if (sr.index != -1) {
            printf("search: found\n");
        } else {
            printf("search: not found\n");
        }
	nanosleep(&wait_time, NULL);
    }
    printf("search: tree state -----------------------------\n");
    showTree(bpt, tid);
    printf("------------------------------------------------\n");
    return NULL;
}

void *delete_test(BPTree *bpt, void *arg) {
    Key key;
    unsigned int seed = SEED_INIT_VAL;
    unsigned char tid = *(unsigned char *) arg;
    key = 1;
    for (int i = 1; i <= loop_times; i++) {
        key = rand_r(&seed) % max_val + 1;
        printf("delete: target = %ld\n", key);
        if (bptreeRemove(bpt, key, tid)) {
            printf("delete: success\n");
        } else {
            printf("delete: failure\n");
        }
    }
    printf("delete: tree state -----------------------------\n");
    showTree(bpt, tid);
    printf("------------------------------------------------\n");
    return NULL;
}

void *warmup(BPTree * bpt, void *args) {
}

int main(int argc, char *argv[]) {
    BPTree *bpt;
    KeyValuePair kv;
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
	printf("loop_times = %d, max_val = %d\n", loop_times, max_val);
    } else {
        printf("default: loop_times = 40, max_val = 1000\n");
    }

    initAllocator(NULL, "data", sizeof(LeafNode) * loop_times / MAX_KEY * 2, 3);

    bpt = newBPTree();

    pthread_t tid[3];

    bptreeThreadInit(BPTREE_BLOCK);
    printf("init\n");

    unsigned char tid1 = 1;
    unsigned char tid2 = 2;
    unsigned char tid3 = 3;
    tid[0] = bptreeCreateThread(bpt, insert_test, warmup, &tid1);
    tid[1] = bptreeCreateThread(bpt, search_test, warmup, &tid2);
    tid[2] = bptreeCreateThread(bpt, delete_test, warmup, &tid3);
    printf("create\n");

    bptreeStartThread();
    printf("start\n");

    bptreeWaitThread(tid[0], NULL);
    bptreeWaitThread(tid[1], NULL);
    bptreeWaitThread(tid[2], NULL);
    printf("wait\n");

    bptreeThreadDestroy();
    printf("destroy\n");

    printf("finish running threads\n");
    showTree(bpt, 0);

    destroyAllocator();

    return 0;
}
