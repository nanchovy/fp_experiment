#include "tree.h"
#include "allocator.h"

#ifdef COUNT_ABORT
__thread unsigned int times_of_lock = 0;
__thread unsigned int times_of_transaction = 0;
unsigned int times_of_lock_sum = 0;
unsigned int times_of_transaction_sum = 0;
__thread unsigned int times_of_abort[4] = {0,0,0,0};
unsigned int times_of_tree_abort[4] = {0,0,0,0};
#endif

#ifdef FREQ_WRITE
unsigned int wrote_size_tmp = 0;
char freq_write_buf[FREQ_WRITE_BUFSZ];
int freq_write_buf_index = 0;
int freq_write_start = 0;
#endif

#ifdef TIME_PART
__thread double internal_alloc_time = 0;
__thread double leaf_alloc_time = 0;
__thread double insert_part1 = 0;
__thread double insert_part2 = 0;
__thread double insert_part3 = 0;
#endif

#ifdef WRITE_AMOUNT
__thread unsigned long write_amount_thr = 0;
unsigned long write_amount = 0;
#endif

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
void show_result_thread(unsigned char tid) {
    pthread_mutex_lock(&mut);
    SHOW_RESULT_THREAD(tid);
    fflush(stderr);
    pthread_mutex_unlock(&mut);
}

#define HTM_INIT()    \
    int n_tries = 1;                    \
    int status = _XABORT_EXPLICIT;      \
    unsigned char method = TRANSACTION; \
    unsigned int wait_times = 1;

#define HTM_TRANSACTION_ABORT() { \
    _xabort(XABORT_STAT);               \
}

#define HTM_RETRY_BY_LOCK(tree, tid) { \
    unlockBPTree(tree, tid);           \
    wait_times = 20000;                \
    while (wait_times--);              \
    continue;                          \
}

#define HTM_TRANSACTION_EXECUTE(tree, code, tid) {   \
    while (1) {                                            \
        if (method == TRANSACTION) {                       \
            if (!tree->lock) {                             \
                status = _xbegin();                        \
            } else {                                       \
                status = _XABORT_EXPLICIT;                 \
            }                                              \
        } else {                                           \
            while (!lockBPTree(tree, tid)) {               \
                do {                                       \
                    _mm_pause();                           \
                } while (tree->lock);                      \
            }                                              \
        }                                                  \
        if (status == _XBEGIN_STARTED || method == LOCK) { \
            {code};                                        \
            if (method == TRANSACTION) {                   \
                _xend();                                   \
                TRANSACTION_SUCCESS();                     \
            } else {                                       \
                unlockBPTree(tree, tid);                   \
                LOCK_SUCCESS();                            \
            }                                              \
            break;                                         \
        } else {                                           \
            ABORT_OCCURRED(status);                       \
            if (tree->lock) {                              \
                do {                                       \
                    _mm_pause();                           \
                } while (tree->lock);                      \
            }                                              \
            if (n_tries < RETRY_NUM) {                     \
                wait_times = 1000 * n_tries;               \
                while (wait_times--);                      \
            } else {                                       \
                method = LOCK;                             \
            }                                              \
            n_tries++;                                     \
        }                                                  \
    }                                                      \
}

#ifndef NPERSIST
void persist(void *target, size_t size) {
    int i;
    for (i = 0; i < (size-1)/64 + 1; i++) {
#  ifdef CLWB
        _mm_clwb(target + i * 64);
#  else
        _mm_clflush(target + i * 64);
#  endif
    }
    WRITE_AMOUNT_REC(((size-1)/64 + 1) * 64);
    _mm_sfence();
}
#else
void persist(void *target, size_t size) { /* EMPTY */ }
#endif

/* utils */
unsigned char hash(Key key) {
    return key % 256;
}

/* initializer */
void initKeyValuePair(KeyValuePair *pair) {
    NVM_WRITE(&pair->key, UNUSED_KEY);
    NVM_WRITE(&pair->value, INITIAL_VALUE);
}

void initLeafNode(LeafNode *node, unsigned char tid) {
    int i;
    ppointer new_pleaf_p = pst_mem_allocate(sizeof(PersistentLeafNode), tid);
    PersistentLeafNode *new_pleaf = (PersistentLeafNode *)getTransientAddr(new_pleaf_p);
    for (i = 0; i < BITMAP_SIZE; i++) {
        NVM_WRITE(&new_pleaf->header.bitmap[i], 0);
    }
    NVM_WRITE(&new_pleaf->header.pnext, P_NULL);
    for (i = 0; i < MAX_PAIR; i++) {
        NVM_WRITE(&new_pleaf->header.fingerprints[i], 0);
        initKeyValuePair(&new_pleaf->kv[i]);
    }
    NVM_WRITE(&new_pleaf->lock, 0);
    node->pleaf = new_pleaf;
    node->next = NULL;
    node->prev = NULL;
    // node->key_length = 0;
    node->tid = tid;
}

void initInternalNode(InternalNode *node) {
    int i;
    node->key_length = 0;
    node->children_type = LEAF;
    for (i = 0; i < MAX_KEY; i++) {
        node->keys[i] = UNUSED_KEY;
    }
    for (i = 0; i < MAX_DEG; i++) {
        node->children[i] = NULL;
    }
}

void insertFirstLeafNode(BPTree *tree, LeafNode *leafHead) {
    if (leafHead != NULL) {
        NVM_WRITE(tree->pmem_head, getPersistentAddr(leafHead->pleaf));
    } else {
        NVM_WRITE(tree->pmem_head, P_NULL);
    }
    tree->head = leafHead;
    tree->root->children_type = LEAF;
    tree->root->children[0] = leafHead;
}

void initBPTree(BPTree *tree, LeafNode *leafHead, InternalNode *rootNode, ppointer *pmem_head) {
    tree->pmem_head = pmem_head;
    tree->root = rootNode;
    insertFirstLeafNode(tree, leafHead);
    tree->lock = 0;
}

void initSearchResult(SearchResult *sr) {
    sr->node = NULL;
    sr->index = -1;
}

LeafNode *newLeafNode(unsigned char tid) {
    INIT_TIME_VAR();
    START_MEJOR_TIME();
    LeafNode *new = (LeafNode *)vol_mem_allocate(sizeof(LeafNode));
    initLeafNode(new, tid);
    FINISH_MEJOR_TIME(LEAF_ALLOC_TIME);
    return new;
}
void destroyLeafNode(LeafNode *node, unsigned char tid) {
    pst_mem_free(getPersistentAddr(node->pleaf), node->tid, tid);
}

InternalNode *newInternalNode() {
    INIT_TIME_VAR();
    START_MEJOR_TIME();
    InternalNode *new = (InternalNode *)vol_mem_allocate(sizeof(InternalNode));
    initInternalNode(new);
    FINISH_MEJOR_TIME(INTERNAL_ALLOC_TIME);
    return new;
}
void destroyInternalNode(InternalNode *node) {
    vol_mem_free(node);
}

BPTree *newBPTree() {
    ppointer *pmem_head = root_allocate(sizeof(ppointer) + (64 - sizeof(ppointer) % 64), sizeof(PersistentLeafNode));
    BPTree *new = (BPTree *)vol_mem_allocate(sizeof(BPTree));
    InternalNode *rootNode = newInternalNode();
    // LeafNode *leafHead = newLeafNode();
    initBPTree(new, NULL, rootNode, pmem_head);
    return new;
}
void destroyBPTree(BPTree *tree, unsigned char tid) {
    // destroyLeafNode(tree->head, tid);
    destroyInternalNode(tree->root);
    *tree->pmem_head = P_NULL;
    root_free(tree->pmem_head);
    vol_mem_free(tree);
    SHOW_WRITE_COUNT();
    SHOW_FREQ_WRITE();
    SHOW_COUNT_ABORT();
    SHOW_WRITE_AMOUNT();
}

int lockLeaf(LeafNode *target, unsigned char tid) {
    if (target->pleaf->lock) {
        return 0;
    } else {
        if (_xtest()) {
            target->pleaf->lock = tid;
            return 1;
        } else {
            return __sync_bool_compare_and_swap(&target->pleaf->lock, 0, tid);
        }
    }
}

int unlockLeaf(LeafNode *target, unsigned char tid) {
    target->pleaf->lock = 0;
    return 1;
}

int lockBPTree(BPTree *target, unsigned char tid) {
    return __sync_bool_compare_and_swap(&target->lock, 0, tid);
}

int unlockBPTree(BPTree *target, unsigned char tid) {
    return __sync_bool_compare_and_swap(&target->lock, tid, 0);
}

int leaf_length(LeafNode *leaf) {
    int i, length = 0;
    for (i = 0; i < BITMAP_SIZE; i++) {
        length += __builtin_popcount(leaf->pleaf->header.bitmap[i]);
        // printf("i = %d, length = %d, bitmaps = %x\n", i, length, leaf->pleaf->header.bitmap[i]);
    }
    return length;
}

int searchInLeaf(LeafNode *node, Key key) {
    int i;
    unsigned char key_fp = hash(key);
    if (node == NULL) {
        return -1;
    }
    for (i = 0; i < MAX_PAIR; i++) {
        if (GET_BIT(node->pleaf->header.bitmap, i) != 0 &&
                node->pleaf->header.fingerprints[i] == key_fp &&
                node->pleaf->kv[i].key == key) {
            return i;
        }
    }
    return -1;
}

int searchInInternal(InternalNode *node, Key target) {
    int i;
    for (i = 0; i < node->key_length; i++) {
        if (target <= node->keys[i]) {
            return i;
        }
    }
    return i;
}

LeafNode *findLeaf(InternalNode *current, Key target_key, InternalNode **parent, unsigned char *retry_flag) {
    *retry_flag = 0;
    if (current->children[0] == NULL) {
        // empty tree
        return NULL;
    }
    int key_index = searchInInternal(current, target_key);
    if (current->children_type == LEAF) {
        if (((LeafNode *)current->children[key_index])->pleaf->lock == 1) {
            *retry_flag = 1;
            return NULL;
        }
        if (parent != NULL) {
            *parent = current;
        }
        return (LeafNode *)current->children[key_index];
    } else {
        current = current->children[key_index];
        return findLeaf(current, target_key, parent, retry_flag);
    }
}

/* search for targetkey in btree.
 * if it was not found, returns -1 index.
 * otherwise, returns nodelist and index of target pair.
 * destroySearchResult should be called after use.
 */
void search(BPTree *bpt, Key target_key, SearchResult *sr, unsigned char tid) {
    int i;
    initSearchResult(sr);

#ifdef DEBUG
    printf("search: key = %ld\n", target_key);
#endif

    if (bpt == NULL) {
#ifdef DEBUG
        printf("search: empty\n");
#endif
        return;
    } else {
        HTM_INIT();
        HTM_TRANSACTION_EXECUTE(bpt,
            unsigned char retry_flag = 0;
            sr->node = findLeaf(bpt->root, target_key, NULL, &retry_flag);
	        if (sr->node != NULL) {
                sr->index = searchInLeaf(sr->node, target_key);
	        } else if (retry_flag) {
                HTM_RETRY_BY_LOCK(bpt, tid);
            }
        , tid);
    }
}
 
int findFirstAvailableSlot(LeafNode *target) {
    int i;
    for (i = 0; i < MAX_PAIR; i++) {
        if (GET_BIT(target->pleaf->header.bitmap, i) == 0) {
            break;
        }
    }
    return i;
}

int compareKeyPositionPair(const void *a, const void *b) {
    return ((KeyPositionPair *)a)->key - ((KeyPositionPair *)b)->key;
}

void findSplitKey(LeafNode *target, Key *split_key, char *bitmap) {
    int i;
    KeyPositionPair pairs[MAX_PAIR];

    // キーは全て有効（満杯の葉が入ってくるので）
    for (i = 0; i < MAX_PAIR; i++) {
        pairs[i].key = target->pleaf->kv[i].key;
        pairs[i].position = i;
    }

    for (i = 0; i < BITMAP_SIZE; i++) {
        bitmap[i] = 0;
    }

    qsort(pairs, MAX_PAIR, sizeof(KeyPositionPair), compareKeyPositionPair);
#ifdef DEBUG
    for (i = 0; i < MAX_PAIR; i++) {
        printf("sorted[%d] = %ld\n", i, pairs[i].key);
    }
#endif

    *split_key = pairs[MAX_PAIR/2-1].key; // this becomes parent key

    for (i = MAX_PAIR/2; i < MAX_PAIR; i++) {
        SET_BIT(bitmap, pairs[i].position);
    }
#ifdef DEBUG
    printf("leaf splitted at %ld\n", *split_key);
    for (i = 0; i < BITMAP_SIZE; i++) {
        printf("bitmap[%d] = %x\n", i, bitmap[i]);
    }
#endif
}

Key splitLeaf(LeafNode *target, KeyValuePair newkv, unsigned char tid, LeafNode *new_leafnode) {
    if (new_leafnode == NULL) {
        new_leafnode = newLeafNode(tid);
    }
    int i;
    Key split_key;
    char bitmap[BITMAP_SIZE];

    for (i = 0; i < MAX_PAIR; i++) {
        new_leafnode->pleaf->kv[i] = target->pleaf->kv[i];
        new_leafnode->pleaf->header.fingerprints[i] = target->pleaf->header.fingerprints[i];
    }
    new_leafnode->pleaf->header.pnext = target->pleaf->header.pnext;
    new_leafnode->pleaf->lock = target->pleaf->lock;
    persist(new_leafnode->pleaf, sizeof(PersistentLeafNode));

    findSplitKey(new_leafnode, &split_key, bitmap);

    for (i = 0; i < BITMAP_SIZE; i++) {
        new_leafnode->pleaf->header.bitmap[i] = bitmap[i];
    }
    persist(new_leafnode->pleaf->header.bitmap, BITMAP_SIZE * sizeof(bitmap[0]));

    for (i = 0; i < BITMAP_SIZE; i++) {
        target->pleaf->header.bitmap[i] = ~bitmap[i];
    }
    persist(target->pleaf->header.bitmap, BITMAP_SIZE * sizeof(bitmap[0]));

    target->pleaf->header.pnext = getPersistentAddr(new_leafnode->pleaf);

    persist(getTransientAddr(target->pleaf->header.pnext), sizeof(LeafNode *));

    new_leafnode->next = target->next;
    new_leafnode->prev = target;
    if (target->next != NULL) {
        target->next->prev = new_leafnode;
    }
    target->next = new_leafnode;

    // insert new node
    if (newkv.key <= split_key) {
        insertNonfullLeaf(target, newkv);
    } else {
        insertNonfullLeaf(new_leafnode, newkv);
    }

    return split_key;
}

Key splitInternal(InternalNode *target, InternalNode **splitted_node, void *new_node, Key new_key) {
    int i;
    int split_position = -1;
    if (new_key <= target->keys[MAX_KEY/2]) {
        split_position = MAX_KEY/2;
    } else {
        split_position = MAX_KEY/2 + 1;
    }
    InternalNode *new_splitted_node = newInternalNode();
    new_splitted_node->key_length = MAX_KEY - (split_position+1);
    int newnode_length = new_splitted_node->key_length;

    for (i = 0; i < newnode_length; i++) {
        new_splitted_node->keys[i] = target->keys[i+split_position+1];
        new_splitted_node->children[i] = target->children[i+split_position+1];
        target->keys[i+split_position+1] = UNUSED_KEY;
        target->children[i+split_position+1] = NULL;
    }
    new_splitted_node->children[i] = target->children[i+split_position+1];
    target->children[i+split_position+1] = NULL;

    new_splitted_node->children_type = target->children_type;

    Key split_key = target->keys[split_position];
    target->keys[split_position] = UNUSED_KEY;
    target->key_length -= newnode_length + 1;

    *splitted_node = new_splitted_node;

    if (new_key <= split_key) {
        insertNonfullInternal(target, new_key, new_node);
    } else {
        insertNonfullInternal(new_splitted_node, new_key, new_node);
    }

    return split_key;
}

// assumption: child is right hand of key
void insertNonfullInternal(InternalNode *target, Key key, void *child) {
    int i;
    for (i = target->key_length; 0 < i && key < target->keys[i-1]; i--) {
        target->keys[i] = target->keys[i-1];
        target->children[i+1] = target->children[i];
    }
    target->children[i+1] = child;
    target->keys[i] = key;
    target->key_length++;
}

void insertNonfullLeaf(LeafNode *node, KeyValuePair kv) {
    int slot = findFirstAvailableSlot(node);
    node->pleaf->kv[slot] = kv;
    node->pleaf->header.fingerprints[slot] = hash(kv.key);
    persist(&node->pleaf->kv[slot], sizeof(KeyValuePair));
    persist(&node->pleaf->header.fingerprints[slot], sizeof(char));
    SET_BIT(node->pleaf->header.bitmap, slot);
    persist(&node->pleaf->header.bitmap[slot/8], sizeof(char));
}

int insert(BPTree *bpt, KeyValuePair kv, unsigned char tid) {
    INIT_TIME_VAR();
    if (bpt == NULL) {
        return 0;
    } else if (bpt->root->children[0] == NULL) {
        START_MEJOR_TIME();
        while (!lockBPTree(bpt, tid)) {
            _mm_pause();
        }
        if (bpt->root->children[0] == NULL) {
                LeafNode *new_leaf = newLeafNode(tid);
                insertFirstLeafNode(bpt, new_leaf);
                insertNonfullLeaf(new_leaf, kv);
        }
        unlockBPTree(bpt, tid);
        FINISH_MEJOR_TIME(INSERT_1_TIME);
        return 1;
    }

    InternalNode *parent;
    unsigned char found_flag = 0;
    LeafNode *target_leaf;
    START_MEJOR_TIME();
    HTM_INIT();
    HTM_TRANSACTION_EXECUTE(bpt,
        unsigned char retry_flag = 0;
        target_leaf = findLeaf(bpt->root, kv.key, &parent, &retry_flag);
        if (retry_flag) {
            HTM_TRANSACTION_ABORT();
            HTM_RETRY_BY_LOCK(bpt, tid);
        }
        if (searchInLeaf(target_leaf, kv.key) != -1) {
            found_flag = 1;
        } else {
            found_flag = 0;
        }
        if (!found_flag && target_leaf != NULL && !lockLeaf(target_leaf, tid)) {
            HTM_TRANSACTION_ABORT();
            HTM_RETRY_BY_LOCK(bpt, tid);
        }
    , tid);
    FINISH_MEJOR_TIME(INSERT_2_TIME);
    if (found_flag) {
        return 0;
    }

#ifdef DEBUG
    printf("locked   %p: %d, %x\n", target_leaf, target_leaf->pleaf->lock, tid);
#endif
    START_MEJOR_TIME();
    if (leaf_length(target_leaf) < MAX_PAIR) {
        insertNonfullLeaf(target_leaf, kv);
    } else {
        Key split_key = splitLeaf(target_leaf, kv, tid, NULL);
        LeafNode *new_leaf = target_leaf->next;
        HTM_TRANSACTION_EXECUTE(bpt,
            insertParent(bpt, parent, split_key, new_leaf, target_leaf);
        , tid);
        unlockLeaf(new_leaf, tid);
    }
#ifdef DEBUG
    printf("unlocked %p: %d, %x\n", target_leaf, target_leaf->pleaf->lock, tid);
#endif
    unlockLeaf(target_leaf, tid);
    FINISH_MEJOR_TIME(INSERT_3_TIME);
    return 1;
}

int bptreeUpdate(BPTree *bpt, KeyValuePair new_kv, unsigned char tid) {
    if (bpt == NULL) {
        return 0;
    } else {
        HTM_INIT();
        unsigned char split_flag = 0;
        unsigned char not_found_flag = 0;
        Key split_key = UNUSED_KEY;
        int target_index = -1;
        LeafNode *splitted_leaf = NULL;
        LeafNode *new_leaf = NULL;
        InternalNode *parent = NULL;
        LeafNode *target = NULL;
        HTM_TRANSACTION_EXECUTE(bpt,
            unsigned char retry_flag = 0;
            target = findLeaf(bpt->root, new_kv.key, &parent, &retry_flag);
            if (target == NULL) {
                not_found_flag = 1;
            } else {
                if (retry_flag) {
                    HTM_TRANSACTION_ABORT();
                    HTM_RETRY_BY_LOCK(bpt, tid);
                }
                lockLeaf(target, tid);
                target_index = searchInLeaf(target, new_kv.key);
                if (target_index == -1) {
                    not_found_flag = 1;
                    unlockLeaf(target, tid);
                }
            }
        , tid);
        if (not_found_flag) {
            return 0;
        }
        if (leaf_length(target) >= MAX_PAIR) {
            new_leaf = newLeafNode(tid);
            KeyValuePair dummy = {UNUSED_KEY, INITIAL_VALUE};
            split_key = splitLeaf(target, dummy, tid, new_leaf);
            splitted_leaf = target;
            if (new_kv.key > split_key) {
                target = new_leaf;
            }
            target_index = searchInLeaf(target, new_kv.key);
        }
        int empty_slot_index = findFirstAvailableSlot(target);
        target->pleaf->kv[empty_slot_index] = new_kv;
        target->pleaf->header.fingerprints[empty_slot_index] = hash(new_kv.key);
        persist(&target->pleaf->kv[empty_slot_index], sizeof(KeyValuePair));
        persist(&target->pleaf->header.fingerprints[empty_slot_index], sizeof(char));
        unsigned char tmp_bitmap[BITMAP_SIZE];
        int i;
        memcpy(tmp_bitmap, target->pleaf->header.bitmap, BITMAP_SIZE);
        CLR_BIT(tmp_bitmap, target_index);
        SET_BIT(tmp_bitmap, empty_slot_index);
        memcpy(target->pleaf->header.bitmap, tmp_bitmap, BITMAP_SIZE);
        persist(target->pleaf->header.bitmap, BITMAP_SIZE);
        if (splitted_leaf != NULL) {
            HTM_TRANSACTION_EXECUTE(bpt,
                insertParent(bpt, parent, split_key, new_leaf, splitted_leaf);
            , tid);
            unlockLeaf(new_leaf, tid);
            unlockLeaf(splitted_leaf, tid);
        } else {
            unlockLeaf(target, tid);
        }
    }
    return 1;
}

int searchNodeInInternalNode(InternalNode *parent, void *target) {
    int i;
    for (i = 0; i <= parent->key_length; i++) {
        if (parent->children[i] == target) {
            return i;
        }
    }
    return -1;
}

void insertParent(BPTree *bpt, InternalNode *parent, Key new_key, LeafNode *new_leaf, LeafNode *target_leaf) {
    if (parent->key_length < MAX_KEY && searchNodeInInternalNode(parent, target_leaf) != -1) {
        insertNonfullInternal(parent, new_key, new_leaf);
    } else {
        Key split_key;
        InternalNode *split_node;
        int splitted = insertRecursive(bpt->root, new_key, new_leaf, &split_key, &split_node);
        if (splitted) {
            // need to update root
	    InternalNode *new_root = newInternalNode();
            new_root->children[0] = bpt->root;
            insertNonfullInternal(new_root, split_key, split_node);
            new_root->children_type = INTERNAL;
            bpt->root = new_root;
        }
    }
}

// new_node is right hand of new_key
int insertRecursive(InternalNode *current, Key new_key, LeafNode *new_node, Key *split_key, InternalNode **split_node) {
    int i;
    if (current->children_type == LEAF) {
        if (current->key_length < MAX_KEY) {
            insertNonfullInternal(current, new_key, new_node);
            return 0;
        } else {
            // this need split
            *split_key = splitInternal(current, split_node, new_node, new_key);
            return 1;
        }
    } else {
        int next_pos = searchInInternal(current, new_key);
        int splitted = insertRecursive(current->children[next_pos], new_key, new_node, split_key, split_node);
        if (splitted) {
            if (current->key_length < MAX_KEY) {
                insertNonfullInternal(current, *split_key, *split_node);
                return 0;
            } else {
                InternalNode *new_split_node;
                Key new_split_key = splitInternal(current, &new_split_node, *split_node, *split_key);
                *split_key = new_split_key;
                *split_node = new_split_node;
                return 1;
            }
        }
        return 0;
    }
}

void deleteLeaf(BPTree *bpt, LeafNode *current, unsigned char tid) {
    if (current->prev == NULL) {
        bpt->head = current->next;
        *bpt->pmem_head = current->pleaf->header.pnext;
        persist(bpt->pmem_head, sizeof(ppointer));
    } else {
        current->prev->pleaf->header.pnext = current->pleaf->header.pnext;
        current->prev->next = current->next;
    }
    if (current->next != NULL) {
        current->next->prev = current->prev;
    }
    destroyLeafNode(current, tid);
}

InternalNode *collapseRoot(InternalNode *oldroot) {
    if (oldroot->children_type == LEAF) {
        return NULL;
    } else {
        return (InternalNode *)oldroot->children[0];
    }
}

void removeEntry(InternalNode *parent, int node_index, Key *right_anchor_key) {
    int key_index = node_index;
    if (parent->key_length != 0 && node_index == parent->key_length) {
        if (right_anchor_key != NULL) {
            *right_anchor_key = parent->keys[parent->key_length - 1];
        }
        key_index--;
    }
    int i;
    for (i = key_index; i < parent->key_length-1; i++) {
        parent->keys[i] = parent->keys[i+1];
    }
    parent->keys[i] = UNUSED_KEY;
    for (i = node_index; i < parent->key_length; i++) {
        parent->children[i] = parent->children[i+1];
    }
    parent->children[i] = NULL;
    parent->key_length--;
}

void shiftToRight(InternalNode *target_node, Key *anchor_key, InternalNode *left_node) {
    int move_length = (target_node->key_length + left_node->key_length)/2 - target_node->key_length;
#ifdef DEBUG
    printf("move_length:%d\n", move_length);
#endif
    int i;
    for (i = target_node->key_length; 0 < i; i--) {
        target_node->keys[i + move_length - 1] = target_node->keys[i - 1];
        target_node->children[i + move_length] = target_node->children[i];
#ifdef DEBUG
        printf("target_node->keys[%d] <- [%d]:%ld\n", i + move_length - 1, i - 1, target_node->keys[i - 1]);
        printf("target_node->children[%d] <- [%d]\n", i + move_length, i);
#endif
    }
    target_node->children[i + move_length] = target_node->children[i];
#ifdef DEBUG
    printf("target_node->children[%d] <- [%d]\n", i + move_length, i);
#endif

    target_node->keys[move_length - 1] = *anchor_key;
#ifdef DEBUG
    printf("target_node->keys[%d] = *anchor_key\n", move_length - 1);
#endif

    for (i = move_length - 1; 0 < i; i--) {
        target_node->keys[i - 1] = left_node->keys[left_node->key_length - move_length + i];
        target_node->children[i] = left_node->children[left_node->key_length - move_length + i + 1];
        left_node->keys[left_node->key_length - move_length + i] = UNUSED_KEY;
        left_node->children[left_node->key_length - move_length + i + 1] = NULL;
#ifdef DEBUG
        printf("target_node->keys[%d] <- left_node->keys[%d]\n", i - 1, left_node->key_length - move_length + i);
        printf("target_node->children[%d] <- left_node->children[%d]\n", i, left_node->key_length - move_length + i + 1);
#endif
    }
    *anchor_key = left_node->keys[left_node->key_length - move_length + i];
    target_node->children[i] = left_node->children[left_node->key_length - move_length + i + 1];
    left_node->keys[left_node->key_length - move_length + i] = UNUSED_KEY;
    left_node->children[left_node->key_length - move_length + i + 1] = NULL;
#ifdef DEBUG
    printf("*anchor_key <- left_node->keys[%d]\n", left_node->key_length - move_length + i);
    printf("target_node->children[%d] <- left_node->children[%d]\n", i, left_node->key_length - move_length + i + 1);
#endif

    left_node->key_length -= move_length;
    target_node->key_length += move_length;
}

void shiftToLeft(InternalNode *target_node, Key *anchor_key, InternalNode *right_node) {
    int move_length = (target_node->key_length + right_node->key_length)/2 - target_node->key_length;
    target_node->keys[target_node->key_length] = *anchor_key;
    *anchor_key = right_node->keys[move_length-1];
    int i;
    for (i = 0; i < move_length-1; i++) {
        target_node->keys[target_node->key_length + 1 + i] = right_node->keys[i];
        target_node->children[target_node->key_length + 1 + i] = right_node->children[i];
#ifdef DEBUG
        printf("move:%ld to [%d]\n", right_node->keys[i], target_node->key_length + 1 + i);
        printf("move:%p to [%d]\n", right_node->children[i], target_node->key_length + 1 + i);
#endif
    }
    target_node->children[target_node->key_length + 1 + i] = right_node->children[i];
#ifdef DEBUG
    printf("move:%p to [%d]\n", right_node->children[i], target_node->key_length + 1 + i);
#endif
    for (i = 0; i < right_node->key_length - move_length; i++) {
        right_node->keys[i] = right_node->keys[i + move_length];
        right_node->children[i] = right_node->children[i + move_length];
#ifdef DEBUG
        printf("left-justify: keys[%d] -> keys[%d]\n", i + move_length, i);
        printf("left-justify: children[%d] -> children[%d]\n", i + move_length, i);
#endif
        right_node->keys[i + move_length] = UNUSED_KEY;
        right_node->children[i + move_length] = NULL;
    }
    right_node->children[i] = right_node->children[i + move_length];
#ifdef DEBUG
    printf("left-justify: children[%d] -> children[%d]\n", i + move_length, i);
#endif
    right_node->children[i + move_length] = NULL;
    right_node->key_length -= move_length;
    target_node->key_length += move_length;
}

void mergeWithLeft(InternalNode *target_node, InternalNode *left_node, Key *anchor_key, Key new_anchor_key) {
    int i;
    left_node->keys[left_node->key_length] = *anchor_key;

    for (i = 0; i < target_node->key_length; i++) {
        left_node->keys[left_node->key_length + 1 + i] = target_node->keys[i];
        left_node->children[left_node->key_length + 1 + i] = target_node->children[i];
    }
    left_node->children[left_node->key_length + 1 + i] = target_node->children[i];
    left_node->key_length += target_node->key_length + 1;

    *anchor_key = new_anchor_key;
}

void mergeWithRight(InternalNode *target_node, InternalNode *right_node, Key *anchor_key, Key new_anchor_key) {
    int i;

    for (i = right_node->key_length; 0 < i; i--) {
        right_node->keys[i + target_node->key_length] = right_node->keys[i - 1];
        right_node->children[i + 1 + target_node->key_length] = right_node->children[i];
#ifdef DEBUG
        printf("make-space:keys[%d] to keys[%d]\n", i - 1, target_node->key_length + i);
        printf("make-space:children[%d] to children[%d]\n", i, i + target_node->key_length + 1);
#endif
    }
    right_node->children[i + 1 + target_node->key_length] = right_node->children[i];
#ifdef DEBUG
    printf("make-space:children[%d] to children[%d]\n", i, i + target_node->key_length + 1);
#endif

    for (i = 0; i < target_node->key_length; i++) {
        right_node->keys[i] = target_node->keys[i];
        right_node->children[i] = target_node->children[i];
#ifdef DEBUG
        printf("fill:%ld -> keys[%d]\n", target_node->keys[i], i);
        printf("fill:%p -> children[%d]\n", target_node->children[i], i);
#endif
    }
    right_node->keys[i] = *anchor_key;
    right_node->children[i] = target_node->children[i];
#ifdef DEBUG
    printf("fill:%ld -> keys[%d]\n", *anchor_key, i);
    printf("fill:%p -> children[%d]\n", target_node->children[i], i);
#endif
    right_node->key_length += target_node->key_length + 1;

    *anchor_key = new_anchor_key;
}

// return 1 when deletion occured
int removeRecursive(BPTree *bpt, InternalNode *current, LeafNode *delete_target_leaf, Key delete_target_key,
                    InternalNode *left_sibling, Key *left_anchor_key, InternalNode *right_sibling, Key *right_anchor_key) {
    int need_rebalance = 0;
    if (current->children_type == LEAF) {
        int leaf_pos = searchNodeInInternalNode(current, delete_target_leaf);
        removeEntry(current, leaf_pos, right_anchor_key);
        return 1;
    } else {
        int next_node_index = searchInInternal(current, delete_target_key);
        if (0 < next_node_index) {
            left_sibling = current->children[next_node_index-1];
            left_anchor_key = &current->keys[next_node_index-1];
        } else {
            if (left_sibling != NULL) {
                left_sibling = left_sibling->children[left_sibling->key_length];
            }
        }
        if (next_node_index < current->key_length) {
            right_sibling = current->children[next_node_index+1];
            right_anchor_key = &current->keys[next_node_index];
        } else {
            if (right_sibling != NULL) {
                right_sibling = right_sibling->children[0];
            }
        }
        InternalNode *next_current = current->children[next_node_index];
        int removed = removeRecursive(bpt, next_current, delete_target_leaf, delete_target_key,
                                      left_sibling, left_anchor_key, right_sibling, right_anchor_key);
        if (removed && next_current->key_length < MIN_KEY) {
            int left_length = 0;
            int right_length = 0;
            // child need rebalancing
            if (left_sibling != NULL) {
                left_length = left_sibling->key_length;
            }
            if (right_sibling != NULL) {
                right_length = right_sibling->key_length;
            }
            if (left_length >= MIN_KEY + 1) {
                shiftToRight(next_current, left_anchor_key, left_sibling);
                return 0;
            } else if (right_length >= MIN_KEY + 1) {
                shiftToLeft(next_current, right_anchor_key, right_sibling);
                return 0;
            } else if (left_length != 0 && left_length + next_current->key_length <= MAX_KEY) {
                // new left anchor key is right anchor key
                if (right_anchor_key != NULL) {
                    mergeWithLeft(next_current, left_sibling, left_anchor_key, *right_anchor_key);
                } else {
                    mergeWithLeft(next_current, left_sibling, left_anchor_key, UNUSED_KEY);
                }
                removeEntry(current, next_node_index, right_anchor_key);
                return 1;
            } else if (right_length != 0 && right_length + next_current->key_length <= MAX_KEY) {
                // new right anchor key is left anchor key
                if (left_anchor_key != NULL) {
                    mergeWithRight(next_current, right_sibling, right_anchor_key, *left_anchor_key);
                } else {
                    mergeWithRight(next_current, right_sibling, right_anchor_key, UNUSED_KEY);
                }
                removeEntry(current, next_node_index, right_anchor_key);
                return 1;
            } else {
                printf("delete: suspicious execution\n");
                return -1; // bug?
            }
        } else {
            return 0;
        }
    }
}

void removeFromParent(BPTree *bpt, InternalNode *parent, LeafNode *target_node, Key target_key) {
    int removed = 0;
    int node_pos;
    if (parent->key_length >= MIN_KEY + 1 && (node_pos = searchNodeInInternalNode(parent, target_node)) != -1 && node_pos < parent->key_length) {
        removeEntry(parent, node_pos, NULL);
    } else {
        removed = removeRecursive(bpt, bpt->root, target_node, target_key, NULL, NULL, NULL, NULL);
    }

    if (removed == 1 && bpt->root->key_length == 0) {
        InternalNode *new_root = collapseRoot(bpt->root);
        if (new_root != NULL) {
            bpt->root = new_root;
        }
    }
    if (bpt->root->key_length == -1) {
        // deleted last node
        bpt->root->key_length = 0;
    }
}

int bptreeRemove(BPTree *bpt, Key target_key, unsigned char tid) {
    SearchResult sr;
    InternalNode *parent = NULL;
    unsigned char exist_flag = 0;
    unsigned char empty_flag = 0;
    LeafNode *target_leaf = NULL;
    HTM_INIT();
    HTM_TRANSACTION_EXECUTE(bpt,
        unsigned char retry_flag = 0;
        target_leaf = findLeaf(bpt->root, target_key, &parent, &retry_flag);
        if (target_leaf != NULL) {
            exist_flag = 1;
            if (!lockLeaf(target_leaf, tid)) {
                HTM_TRANSACTION_ABORT();
                HTM_RETRY_BY_LOCK(bpt, tid);
            }
            if (leaf_length(target_leaf) == 1) {
                empty_flag = 1;
                if (target_leaf->prev != NULL && !lockLeaf(target_leaf->prev, tid)) {
                    HTM_TRANSACTION_ABORT();
                    unlockLeaf(target_leaf, tid);
                    HTM_RETRY_BY_LOCK(bpt, tid);
                }
            } else {
                empty_flag = 0;
            }
        } else if (retry_flag) {
            HTM_RETRY_BY_LOCK(bpt, tid);
        }
    , tid);
    if (!exist_flag) {
        return 0;
    }

    int keypos = searchInLeaf(target_leaf, target_key);
    if (keypos == -1) {
#ifdef DEBUG
        printf("delete:the key doesn't exist. abort.\n");
#endif
        unlockLeaf(target_leaf, tid);
        if (empty_flag && target_leaf->prev != NULL) {
            unlockLeaf(target_leaf->prev, tid);
        }
        return 0;
    }

    if (empty_flag) {
        if (target_leaf->prev != NULL) {
                HTM_TRANSACTION_EXECUTE(bpt,
                    removeFromParent(bpt, parent, target_leaf, target_key);
                , tid);
                deleteLeaf(bpt, target_leaf, tid); // cannot delete leaf before remove entry
                unlockLeaf(target_leaf->prev, tid);
        } else {
            HTM_TRANSACTION_EXECUTE(bpt,
                removeFromParent(bpt, parent, target_leaf, target_key);
            , tid);
            deleteLeaf(bpt, target_leaf, tid);
        }
        // deleted leaf does not need to be unlocked
    } else {
        CLR_BIT(target_leaf->pleaf->header.bitmap, keypos);
        persist(target_leaf->pleaf->header.bitmap, BITMAP_SIZE);
        unlockLeaf(target_leaf, tid);
    }

    return 1;
}

/* debug function */
void showLeafNode(LeafNode *node, int depth) {
    int i, j;
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("leaf:%p (persistent -> %p)\n", node, node->pleaf);
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("\tnext = %p, prev = %p, pnext = %p\n", node->next, node->prev, getTransientAddr(node->pleaf->header.pnext));
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("\tlength = %d\n", leaf_length(node));
    for (i = 0; i < MAX_PAIR; i++) {
        for (j = 0; j < depth; j++)
            printf("\t");
        printf("\tbitmap[%d]:%d\n", i, GET_BIT(node->pleaf->header.bitmap, i));
    }
    for (i = 0; i < MAX_PAIR; i++) {
        if ((GET_BIT(node->pleaf->header.bitmap, i)) == 0)
            continue;
        for (j = 0; j < depth; j++)
            printf("\t");
        printf("\tkv[%d]:fgp = %d, key = %ld, val = %d\n", i, node->pleaf->header.fingerprints[i], node->pleaf->kv[i].key, node->pleaf->kv[i].value);
    }
}

void showInternalNode(InternalNode *node, int depth) {
    int i, j;
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("internal:%p\n", node);
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("\tlength = %d\n", node->key_length);
    for (i = 0; i < node->key_length; i++) {
        for (j = 0; j < depth; j++)
            printf("\t");
        printf("\tkey[%d] = %ld\n", i, node->keys[i]);
    }
    if (node->children[0] != NULL) {
        for (i = 0; i < node->key_length+1; i++) {
            for (j = 0; j < depth; j++)
                printf("\t");
            printf("\tchildren[%d]:\n", i);
            if (node->children_type == LEAF) {
                showLeafNode((LeafNode *)node->children[i], depth+1);
            } else {
                showInternalNode((InternalNode *)node->children[i], depth+1);
            }
        }
    } else {
        // empty tree
    }
}

void showTree(BPTree *bpt, unsigned char tid) {
    HTM_INIT();
    HTM_TRANSACTION_EXECUTE(bpt,
        printf("leaf head:%p\n", bpt->head);
        showInternalNode((InternalNode *)bpt->root, 0);
    , tid);
}
