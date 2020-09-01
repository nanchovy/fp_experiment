#ifdef CONCURRENT
#  error "CONCURRENT is defined!"
#endif
#include "tree.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    BPTree *bpt;
    KeyValuePair kv;
#ifdef BPTREE
    initAllocator(NULL, "data", sizeof(LeafNode) + 100, 1);
#else
    initAllocator(NULL, "data", sizeof(PersistentLeafNode) + 100, 1);
#endif
    bpt = newBPTree();

    kv.key = 1;
    kv.value = 1;
    insert(bpt, kv, 0);

    showTree(bpt, 0);
    destroyBPTree(bpt, 0);
    destroyAllocator();

    return 0;
}
