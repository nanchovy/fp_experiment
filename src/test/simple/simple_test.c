#ifdef CONCURRENT
#  error "CONCURRENT is defined!"
#endif
#include "tree.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    initAllocator(NULL, "data", sizeof(LeafNode), 1);
    BPTree *bpt = newBPTree();
    KeyValuePair kv;
    kv.key = 1;
    kv.value = 1;
    srand(1);

    kv.key = rand();
    printf("insert %ld\n", kv.key);
    insert(bpt, kv, 0);

    SearchResult sr;
    showTree(bpt, 0);
    search(bpt, kv.key, &sr, 0);
    if (sr.index == -1) {
        printf("not found: %ld\n", kv.key);
    }
    printf("search: %ld => (%p, %d)\n", kv.key, sr.node, sr.index);

    printf("delete %ld\n", kv.key);
    bptreeRemove(bpt, kv.key, 0);

    showTree(bpt, 0);
    destroyBPTree(bpt, 0);
    destroyAllocator();
    return 0;
}
