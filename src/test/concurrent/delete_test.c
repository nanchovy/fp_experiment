#ifdef NVHTM
#  error "NVHTM is defined!"
#endif
#include "tree.h"
#include <stdlib.h>
#include <time.h>
#define NUM 142

int main(int argc, char *argv[]) {
    BPTree *bpt;
    KeyValuePair kv;
    int loop_times = 40;
    int max_val = 1000;
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
    initAllocator(NULL, "data", sizeof(PersistentLeafNode) * (loop_times / (MAX_PAIR / 2) + 1) + sizeof(AllocatorHeader), 1);
    bpt = newBPTree();
    kv.key = 1;
    kv.value = 1;
    for (int i = 1; i <= loop_times; i++) {
        kv.key = i;
        printf("insert %ld\n", kv.key);
        if (insert(bpt, kv, 1)) {
            printf("success\n");
        } else {
            printf("failure\n");
        }
    }
    for (int i = 1; i <= loop_times; i++) {
        kv.key = i;
        printf("delete %ld\n", kv.key);
        if (bptreeRemove(bpt, kv.key, 1)) {
            printf("success\n");
        } else {
            printf("failure\n");
            assert(0);
        }
    }
    showTree(bpt, 1);
    srand(loop_times);
    for (int i = 1; i <= loop_times; i++) {
        kv.key = rand() % max_val;
        printf("insert %ld\n", kv.key);
        if (insert(bpt, kv, 1)) {
            printf("success\n");
        } else {
            printf("failure\n");
        }
    }
    srand(loop_times);
    for (int i = 1; i <= loop_times; i++) {
        kv.key = rand() % max_val;
        printf("delete %ld\n", kv.key);
        if (kv.key == 142) {
            showTree(bpt, 1);
        }
        if (bptreeRemove(bpt, kv.key, 1)) {
            printf("success\n");
        } else {
            printf("failure\n");
            SearchResult sr;
            search(bpt, kv.key, &sr, 1);
            printf("sr = (%p, %d)\n", sr.node, sr.index);
            // showTree(bpt, 1);
            if (sr.index != -1) {
                assert(0);
            }
        }
        if (kv.key == 142) {
            showTree(bpt, 1);
        }
    }

    showTree(bpt, 1);
    destroyBPTree(bpt, 1);
    destroyAllocator();

    return 0;
}
