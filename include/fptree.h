#ifndef FPTREE_H
#define FPTREE_H
#  ifdef __cplusplus
extern "C" {
#  endif
#include "nv-htm_wrapper.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <xmmintrin.h>
#ifndef NPERSIST
#  include <x86intrin.h>
#endif
#ifdef CONCURRENT
#  include <immintrin.h>
#  include <sched.h>
#  define XABORT_STAT 0
#  define RETRY_NUM 20 // times of retry RTM. < 32
// #  define LOOP_RETRY_NUM 10 // times of retry GET_LOCK_LOOP
#  define TRANSACTION 1
#  define LOCK 0
#endif
#include "allocator.h"
extern ppointer PADDR_NULL;

#define MIN_KEY 128
#define MIN_DEG (MIN_KEY+1)
#define MAX_KEY (2*MIN_KEY+1)
#define MAX_DEG (MAX_KEY+1)
#define MAX_PAIR 64

#define BITMAP_SIZE ((MAX_PAIR/8)+(MAX_PAIR%8 > 0))

/* definition of structs */

/* bitmap operator */
#define GET_BIT(bitmapaddr, index) (\
    (bitmapaddr[index/8] & (1 << ((index)%8))) >> (index)%8\
)
#define SET_BIT(bitmapaddr, index) ({\
    WRITE_COUNT_UP(); \
    bitmapaddr[index/8] |= (1 << ((index)%8)); \
})
#define CLR_BIT(bitmapaddr, index) ({\
    WRITE_COUNT_UP(); \
    bitmapaddr[index/8] &= ~(1 << ((index)%8)); \
})

#ifdef NVHTM
#  define GET_BIT_T(bitmapaddr, index) (\
    (NVM_read(&bitmapaddr[index/8]) & (1 << ((index)%8))) >> (index)%8\
)
#  define SET_BIT_T(bitmapaddr, index) ({\
    WRITE_COUNT_UP(); \
    char bt_tmp = NVM_read(&bitmapaddr[index/8]) | (1 << ((index)%8));\
    NVM_write_varsize(&bitmapaddr[index/8], &bt_tmp, sizeof(char)); \
})
#  define CLR_BIT_T(bitmapaddr, index) ({\
    WRITE_COUNT_UP(); \
    char bt_tmp = NVM_read(&bitmapaddr[index/8]) & ~(1 << ((index)%8));\
    NVM_write_varsize(&bitmapaddr[index/8], &bt_tmp, sizeof(char)); \
})
#endif

#ifdef TIME_PART
#include <time.h>
extern __thread double internal_alloc_time;
extern __thread double leaf_alloc_time;
extern __thread double insert_part1;
extern __thread double insert_part2;
extern __thread double insert_part3;
#  define INTERNAL_ALLOC_TIME internal_alloc_time
#  define LEAF_ALLOC_TIME leaf_alloc_time
#  define INSERT_1_TIME insert_part1
#  define INSERT_2_TIME insert_part2
#  define INSERT_3_TIME insert_part3
#  define INIT_TIME_VAR()     \
    struct timespec stt, edt; \
    double time_tmp = 0

#  define START_MEJOR_TIME() \
	clock_gettime(CLOCK_MONOTONIC_RAW, &stt)

#  define FINISH_MEJOR_TIME(time_res) {         \
	clock_gettime(CLOCK_MONOTONIC_RAW, &edt); \
	time_tmp = 0;                             \
	time_tmp += (edt.tv_nsec - stt.tv_nsec);  \
	time_tmp /= 1000000000;                   \
	time_tmp += edt.tv_sec - stt.tv_sec;      \
	time_res += time_tmp;                     \
}

#  define SHOW_TIME_PART() {\
    fprintf(stderr, "allocating internal node = %lf\n", internal_alloc_time);\
    fprintf(stderr, "allocating leaf node = %lf\n", leaf_alloc_time);\
	fprintf(stderr, "insert_part1:new root = %lf\n", insert_part1);\
	fprintf(stderr, "insert_part2:find leaf = %lf\n", insert_part2);\
	fprintf(stderr, "insert_part3:update parent = %lf\n", insert_part3);\
}
#else
#  define INTERNAL_ALLOC_TIME
#  define LEAF_ALLOC_TIME
#  define INSERT_1_TIME
#  define INSERT_2_TIME
#  define INSERT_3_TIME
#  define INIT_TIME_VAR()
#  define START_MEJOR_TIME()
#  define FINISH_MEJOR_TIME(res)
#  define SHOW_TIME_PART()
#endif

#  define SHOW_RESULT_THREAD(tid) {\
    fprintf(stderr, "*******************Thread %d*******************\n", tid);\
    SHOW_TIME_PART();\
    SHOW_TRANSACTION_SIZE();\
    SUM_COUNT_ABORT();\
    WRITE_AMOUNT_ADD();\
    fprintf(stderr, "***********************************************\n");\
}
void show_result_thread(unsigned char);

/* structs */

#define LEAF 0
#define INTERNAL 1
// #define LEAF_HEADER_SIZE (BITMAP_SIZE+sizeof(ppointer)+MAX_PAIR)
// #define LEAF_DATA_SIZE (sizeof(KeyValuePair)*MAX_PAIR+1)
// #define LEAF_SIZE (LEAF_HEADER_SIZE+LEAF_DATA_SIZE)

struct InternalNode {
    int key_length;
    unsigned char children_type;
    Key keys[MAX_KEY];
    void *children[MAX_DEG];
};

struct LeafHeader {
    unsigned char bitmap[BITMAP_SIZE];
    ppointer pnext;
    unsigned char fingerprints[MAX_PAIR];
};

struct PersistentLeafNode {
    struct LeafHeader header;
    KeyValuePair kv[MAX_PAIR];
    unsigned char lock;
    char pad[64 - (sizeof(struct LeafHeader) + sizeof(KeyValuePair) * MAX_PAIR + sizeof(unsigned char)) % 64];
};

struct LeafNode {
    unsigned char tid;
    struct PersistentLeafNode *pleaf;
    struct LeafNode *next;
    struct LeafNode *prev;
//    int key_length;
};

struct BPTree {
    struct InternalNode *root;
    ppointer *pmem_head;
    struct LeafNode *head;
    int lock;
};

struct SearchResult {
    struct LeafNode *node;
    int index;
};

struct KeyPositionPair {
    Key key;
    int position;
};

typedef struct PersistentLeafNode PersistentLeafNode;
typedef struct LeafNode LeafNode;
typedef struct InternalNode InternalNode;
typedef struct BPTree BPTree;
typedef struct SearchResult SearchResult;
typedef struct KeyPositionPair KeyPositionPair;

/* utils */
unsigned char hash(Key);
// char popcntcharsize(char);

/* initializer */
void initKeyValuePair(KeyValuePair *);
void initLeafNode(LeafNode *, unsigned char);
void initInternalNode(InternalNode *);
void initBPTree(BPTree *, LeafNode *, InternalNode *, ppointer *);
void initSearchResult(SearchResult *);

LeafNode *newLeafNode(unsigned char);
void destroyLeafNode(LeafNode *, unsigned char);
InternalNode *newInternalNode();
void destroyInternalNode(InternalNode *);
BPTree *newBPTree();
void destroyBPTree(BPTree *, unsigned char);

// int getLeafNodeLength(LeafNode *);
int searchInLeaf(LeafNode *, Key);
LeafNode *findLeaf(InternalNode *, Key, InternalNode **, unsigned char *);
void search(BPTree *, Key, SearchResult *, unsigned char);

int findFirstAvailableSlot(LeafNode *);
int compareKeyPositionPair(const void *, const void *);
void findSplitKey(LeafNode *, Key *, char *);
Key splitLeaf(LeafNode *, KeyValuePair, unsigned char, LeafNode *);
Key splitInternal(InternalNode *, InternalNode **, void *, Key);
void insertNonfullInternal(InternalNode *, Key, void *);
void insertNonfullLeaf(LeafNode *, KeyValuePair);
int insert(BPTree *, KeyValuePair, unsigned char);
int searchNodeInInternalNode(InternalNode *, void *);
void insertParent(BPTree *, InternalNode *, Key, LeafNode *, LeafNode *);
int insertRecursive(InternalNode *, Key, LeafNode *, Key *, InternalNode **);

void insertLeaf(BPTree *, LeafNode *);
int bptreeUpdate(BPTree *, KeyValuePair, unsigned char);

// void deleteLeaf(BPTree *, LeafNode *, LeafNode *);
// void *collapseRoot(void *);
// InternalNode *shift(InternalNode *, InternalNode *, InternalNode *, char, void **);
// InternalNode *merge(InternalNode *, InternalNode *, InternalNode *, char);
// void *rebalance(BPTree *, void *, void *, void *, void *, void *, void *, void **);
// void *findRebalance(BPTree *, void *, void *, void *, void *, void *, void *, Key, void **);
int bptreeRemove(BPTree *, Key, unsigned char);
// 
/* debug function */
void showLeafNode(LeafNode *, int);
void showInternalNode(InternalNode *, int);
void showTree(BPTree *, unsigned char);
#  ifdef __cplusplus
};
#  endif
#endif
