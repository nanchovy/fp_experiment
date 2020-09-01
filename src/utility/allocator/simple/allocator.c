#ifdef CONCURRENT
#  error "CONCURRENT is defined!"
#endif
#include "allocator.h"

int initAllocator(void *ex_p, const char *fn, size_t root_size, unsigned char thread_num) { return 0; }
int destroyAllocator() { return 0; }
ppointer recoverAllocator(ppointer (*getNext)(ppointer)) { return NULL; }

void *vol_mem_allocate(size_t size) {
    return malloc(size);
}
ppointer pst_mem_allocate(size_t size, unsigned char tid) {
    return malloc(size);
}
ppointer *root_allocate(size_t size, size_t element_size) {
    return pst_mem_allocate(size, 0);
}

void vol_mem_free(void *p) {
    free(p);
}
void pst_mem_free(ppointer p, unsigned char node_tid, unsigned char tid) {
    free(p);
}
void root_free(ppointer *p) {
    free(p);
}

ppointer getPersistentAddr(void *p) {return p;}
void *getTransientAddr(ppointer p) {return p;}
