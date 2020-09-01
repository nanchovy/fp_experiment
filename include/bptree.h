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
// #ifdef USE_ASSERTION
#  include <assert.h>
// #endif
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

#define MIN_KEY 128
#define MIN_DEG (MIN_KEY+1)
#define MAX_KEY (2*MIN_KEY+1)
#define MAX_DEG (MAX_KEY+1)
#ifndef MAX_PAIR
#  define MAX_PAIR 128
#endif

#include "allocator.h"
extern ppointer PADDR_NULL;

#  define SHOW_RESULT_THREAD(tid) {\
    fprintf(stderr, "*******************Thread %d*******************\n", tid);\
    SHOW_TRANSACTION_SIZE();\
    SUM_COUNT_ABORT();\
    fprintf(stderr, "***********************************************\n");\
}

//#define BITMAP_SIZE ((MAX_PAIR/8)+1)

/* definition of structs */

/* bitmap operator */
/*
#define GET_BIT(bitmapaddr, index) (\
    (bitmapaddr[index/8] & (1 << ((index)%8))) >> (index)%8\
)
#define SET_BIT(bitmapaddr, index) (\
    bitmapaddr[index/8] |= (1 << ((index)%8)) \
)
#define CLR_BIT(bitmapaddr, index) (\
    bitmapaddr[index/8] &= ~(1 << ((index)%8)) \
)

#ifdef NVHTM
#  define GET_BIT_T(bitmapaddr, index) (\
    (NVM_read(&bitmapaddr[index/8]) & (1 << ((index)%8))) >> (index)%8\
)
#  define SET_BIT_T(bitmapaddr, index) ({\
    char bt_tmp = NVM_read(&bitmapaddr[index/8]) | (1 << ((index)%8));\
    NVM_write_varsize(&bitmapaddr[index/8], &bt_tmp, sizeof(char)); \
})
#  define CLR_BIT_T(bitmapaddr, index) ({\
    char bt_tmp = NVM_read(&bitmapaddr[index/8]) & ~(1 << ((index)%8));\
    NVM_write_varsize(&bitmapaddr[index/8], &bt_tmp, sizeof(char)); \
})
#endif
*/

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

struct LeafNode {
    unsigned char tid;
    struct LeafNode *next;
    struct LeafNode *prev;
    ppointer pnext;
    int key_length;
    KeyValuePair kv[MAX_PAIR];
};

struct BPTree {
    struct InternalNode *root;
//    ppointer *pmem_head;
    struct LeafNode *head;
    int lock;
    ppointer *pmem_head;
};

struct SearchResult {
    struct LeafNode *node;
    int index;
};

struct KeyPositionPair {
    Key key;
    int position;
};

//typedef struct PersistentLeafNode PersistentLeafNode;
typedef struct LeafNode LeafNode;
typedef struct InternalNode InternalNode;
typedef struct BPTree BPTree;
typedef struct SearchResult SearchResult;
typedef struct KeyPositionPair KeyPositionPair;

typedef LeafNode PersistentLeafNode;

/* utils */
unsigned char hash(Key);
// char popcntcharsize(char);
#ifdef TIME_PART
void showTime(unsigned int);
#include <time.h>
#endif

/* initializer */
void initKeyValuePair(KeyValuePair *);
void initLeafNode(LeafNode *, unsigned char);
void initInternalNode(InternalNode *);
//void initBPTree(BPTree *, LeafNode *, InternalNode *, ppointer *);
void initBPTree(BPTree *, LeafNode *, InternalNode *);
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
void findSplitKey(LeafNode *, Key *);
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
