#ifdef NVHTM
#  error "NVHTM is defined!"
#endif
#include "tree.h"
#include <stdlib.h>
int leaf_length(LeafNode *);

int main(int argc, char *argv[]) {
    initAllocator(NULL, "data", 64 + sizeof(AllocatorHeader) + 2 * sizeof(PersistentLeafNode), 1);
    BPTree *bpt = newBPTree();
    LeafNode *leaf1 = newLeafNode(1);
    LeafNode *leaf2 = newLeafNode(1);
    printf("leaf nodes are cache-aligned? 1: %ld, 2: %ld\n", ((intptr_t)leaf1->pleaf) % 64, ((intptr_t)leaf2->pleaf) % 64);
    printf("length = 0?: %d\n", leaf_length(leaf1));
    leaf1->pleaf->header.bitmap[0] |= 1;
    printf("length = 1?: %d\n", leaf_length(leaf1));
    destroyLeafNode(leaf1, 1);
    destroyLeafNode(leaf2, 1);
    destroyBPTree(bpt, 1);
    destroyAllocator();
    return 0;
}
