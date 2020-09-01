#include "tree.h"
#include "allocator.h"
#include "nv-htm_wrapper.h"

typedef intptr_t NVHTM_WRITE_UNIT_T;
const Key UNUSED_KEY = -1;
const Value INITIAL_VALUE = 0;
#ifdef TRANSACTION_SIZE
__thread unsigned int transaction_counter;
__thread unsigned int transaction_counter_max;
#endif
#ifdef COUNT_ABORT
__thread unsigned int times_of_lock;
__thread unsigned int times_of_transaction;
unsigned int times_of_lock_sum;
unsigned int times_of_transaction_sum;
__thread unsigned int times_of_abort[4];
unsigned int times_of_tree_abort[4];
#endif

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
void show_result_thread(unsigned char tid) {
    pthread_mutex_lock(&mut);
    SHOW_RESULT_THREAD(tid);
    fflush(stderr);
    pthread_mutex_unlock(&mut);
}

void persist(void *target, size_t size) { /* EMPTY */ }

/* utils */
unsigned char hash(Key key) {
    return key % 256;
}

void NVM_write_varsize(void *addr_v, const void *value_v, size_t len) {
    char *addr = (char *)addr_v;
    char *value = (char *)value_v;
    extern void *al_pool;
    NVHTM_WRITE_UNIT_T tmp;
    size_t us = sizeof(NVHTM_WRITE_UNIT_T);
    int i;
    for (i = 0; i < len / us; i++) {
        NVM_write(addr + i * us, *(NVHTM_WRITE_UNIT_T *)(value + i * us));
        // memcpy(addr + i * us, (NVHTM_WRITE_UNIT_T *)(value + i * us), sizeof(NVHTM_WRITE_UNIT_T));
    }
    if (len % us != 0) {
        // preserve other bits
        long mask = (0xffffffffffffffff) << (len % us) * 8;
        tmp = *(NVHTM_WRITE_UNIT_T *)(addr + i * us);
        tmp &= mask;
        tmp |= (*(NVHTM_WRITE_UNIT_T *)value) & (~mask);
        NVM_write(addr + i * us, tmp);
        // memcpy(addr + i * us, &tmp, sizeof(NVHTM_WRITE_UNIT_T));
    }
}

/* initializer */
void initKeyValuePair(KeyValuePair *pair) {
    NVM_write_varsize(&pair->key, &UNUSED_KEY, sizeof(Key));
    NVM_write_varsize(&pair->value, &INITIAL_VALUE, sizeof(Value));
}

void initLeafNode(LeafNode *node, unsigned char tid) {
    //int test[1]; printf("%d", test[100]);
    int i;
    char zchar = 0;
    int zint = 0;
    // ここのトランザクションは呼び出し元で実装されていることを前提として削除
    // NVHTM_begin();
    void *null_var = NULL;
    NVM_write_varsize(&node->next, &null_var, sizeof(NULL));
    NVM_write_varsize(&node->prev, &null_var, sizeof(NULL));
    NVM_write_varsize(&node->pnext, &P_NULL, sizeof(P_NULL));
    for (i = 0; i < MAX_PAIR; i++) {
        initKeyValuePair(&node->kv[i]);
    }
    NVM_write_varsize(&node->key_length, &zint, sizeof(int));
    NVM_write_varsize(&node->tid, &tid, sizeof(unsigned char));
    // NVHTM_end();
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

void initBPTree(BPTree *tree, LeafNode *leafHead, InternalNode *rootNode) {
    if (leafHead != NULL) {
        *tree->pmem_head = getPersistentAddr(leafHead);
    } else {
        *tree->pmem_head = P_NULL;
    }
    tree->head = leafHead;
    tree->root = rootNode;
    rootNode->children_type = LEAF;
    rootNode->children[0] = leafHead;
}

void initSearchResult(SearchResult *sr) {
    sr->node = NULL;
    sr->index = -1;
}

LeafNode *newLeafNode(unsigned char tid) {
    ppointer new_pp = pst_mem_allocate(sizeof(LeafNode), tid);
    LeafNode *new = (LeafNode *)getTransientAddr(new_pp);
    initLeafNode(new, tid);
    return new;
}
void destroyLeafNode(LeafNode *node, unsigned char tid) {
    pst_mem_free(getPersistentAddr(node), node->tid, tid);
}

InternalNode *newInternalNode() {
    InternalNode *new = (InternalNode *)vol_mem_allocate(sizeof(InternalNode));
    initInternalNode(new);
    return new;
}
void destroyInternalNode(InternalNode *node) {
    vol_mem_free(node);
}

BPTree *newBPTree() {
    ppointer *pmem_head = root_allocate(sizeof(ppointer), sizeof(LeafNode));
    BPTree *new = (BPTree *)vol_mem_allocate(sizeof(BPTree));
    InternalNode *rootNode = newInternalNode();
    new->pmem_head = pmem_head;
    initBPTree(new, NULL, rootNode);
    return new;
}
void destroyBPTree(BPTree *tree, unsigned char tid) {
    // destroyLeafNode(tree->head, tid);
    destroyInternalNode(tree->root);
    *tree->pmem_head = P_NULL;
    root_free(tree->pmem_head);
    vol_mem_free(tree);
}

int lockLeaf(LeafNode *target, unsigned char tid) {
    return tid;
}

int unlockLeaf(LeafNode *target, unsigned char tid) {
    return tid;
}

int lockBPTree(BPTree *target, unsigned char tid) {
    return tid;
}

int unlockBPTree(BPTree *target, unsigned char tid) {
    return tid;
}

int searchInLeaf(LeafNode *node, Key key) {
    if (node == NULL) {
        return -1;
    }
    /*
    for (int i = 0; i < node->key_length; i++) {
        if (node->kv[i].key == key) {
            return i;
        }
    }
    */
	int left = 0;
	int right = NVM_read(&node->key_length);
	while (left < right) {
        // int mid = (right + left) / 2;
		int mid = left + (right - left) / 2;
        int cur_key = NVM_read(&node->kv[mid].key);
		if (cur_key == key) {
            return mid;
        }
		if (key < cur_key) {
			right = mid;
		} else {
			left = mid + 1;
		}
	}
    return -1;
}

int searchInInternal(InternalNode *node, Key target) {
    int i;
    for (i = 0; i < NVM_read(&node->key_length); i++) {
        if (target <= NVM_read(&node->keys[i])) {
            return i;
        }
    }
    return i;
}

LeafNode *findLeaf(InternalNode *current, Key target_key, InternalNode **parent, unsigned char *retry_flag) {
    if (current->children[0] == NULL) {
        // empty tree
        return NULL;
    }
    int key_index = searchInInternal(current, target_key);
    if (current->children_type == LEAF) {
        if (parent != NULL) {
            *parent = current;
        }
        return (LeafNode *)current->children[key_index];
    } else {
        current = current->children[key_index];
        return findLeaf(current, target_key, parent, NULL);
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
    printf("search: key = %d\n", targetkey);
#endif

    if (bpt == NULL) {
#ifdef DEBUG
        printf("search: empty\n");
#endif
        return;
    } else {
        // while (1) {
        // _xstart();
        NVHTM_begin();
        sr->node = findLeaf(bpt->root, target_key, NULL, NULL);
        if (sr->node != NULL) {
            sr->index = searchInLeaf(sr->node, target_key);
        }
        NVHTM_end();
        // _xend();
        // }
    }
}

int compareKeyPositionPair(const void *a, const void *b) {
    return ((KeyPositionPair *)a)->key - ((KeyPositionPair *)b)->key;
}

void findSplitKey(LeafNode *target, Key *split_key) {
    // showLeafNode(target, 1);
    *split_key = NVM_read(&target->kv[MAX_PAIR/2-1].key); // this becomes parent key
}

Key splitLeaf(LeafNode *target, KeyValuePair newkv, unsigned char tid, LeafNode *new_leafnode) {
    if (new_leafnode == NULL) {
        new_leafnode = newLeafNode(tid);
    }
    
    int split_index = MAX_PAIR/2 - 1;
    Key split_key;
    findSplitKey(target, &split_key);

    KeyValuePair kv_tmp;
    unsigned char c_tmp;
    ppointer pp_tmp;
    LeafNode *leaf_tmp;
    int len_tmp;

    for (int i = 0; i < MAX_PAIR - split_index - 1; i++) {
        kv_tmp = NVM_read(&target->kv[(split_index + 1) + i]);
        NVM_write_varsize(&new_leafnode->kv[i], &kv_tmp, sizeof(KeyValuePair));
        initKeyValuePair(&target->kv[(split_index + 1) + i]);
    }
    pp_tmp = NVM_read(&target->pnext);
    NVM_write_varsize(&new_leafnode->pnext, &pp_tmp, sizeof(ppointer));
    leaf_tmp = NVM_read(&target->next);
    NVM_write_varsize(&new_leafnode->next, &leaf_tmp, sizeof(LeafNode *));
    leaf_tmp = NVM_read(&target);
    NVM_write_varsize(&new_leafnode->prev, &leaf_tmp, sizeof(LeafNode *));
    len_tmp = MAX_PAIR - split_index - 1;
    NVM_write_varsize(&new_leafnode->key_length, &len_tmp, sizeof(int));

    if (NVM_read(&target->next) != NULL) {
        NVM_write_varsize(&target->next->prev, &new_leafnode, sizeof(LeafNode *));
    }
    NVM_write_varsize(&target->next, &new_leafnode, sizeof(LeafNode *));
    pp_tmp = getPersistentAddr(new_leafnode);
    NVM_write_varsize(&target->pnext, &pp_tmp, sizeof(ppointer));
    len_tmp = split_index + 1;
    NVM_write_varsize(&target->key_length, &len_tmp, sizeof(int));

    // insert new node
    if (newkv.key != UNUSED_KEY) {
        if (newkv.key <= split_key) {
            insertNonfullLeaf(target, newkv);
        } else {
            insertNonfullLeaf(new_leafnode, newkv);
        }
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
    int len = NVM_read(&node->key_length);
    if (len == 0) {
        NVM_write_varsize(&node->kv[0], &kv, sizeof(KeyValuePair));
        len = len + 1;
        NVM_write_varsize(&node->key_length, &len, sizeof(int));
        return;
    }
    // make space for insert
    int slot;
    for (slot = len; 0 < slot; slot--) {
        if (NVM_read(&node->kv[slot - 1].key) < kv.key) {
            break;
        }
        KeyValuePair kv_tmp = NVM_read(&node->kv[slot - 1]);
        NVM_write_varsize(&node->kv[slot], &kv_tmp, sizeof(KeyValuePair));
    }
    // insert
    NVM_write_varsize(&node->kv[slot], &kv, sizeof(KeyValuePair));
    len = len + 1;
    NVM_write_varsize(&node->key_length, &len, sizeof(int));
}

int insert(BPTree *bpt, KeyValuePair kv, unsigned char tid) {
    if (bpt == NULL) {
        return 0;
    }
    NVHTM_begin();
    if (bpt->root->children[0] == NULL) {
        lockBPTree(bpt, tid);
        LeafNode *new_leaf = newLeafNode(tid);
        initBPTree(bpt, new_leaf, bpt->root);
        insertNonfullLeaf(new_leaf, kv);
        unlockBPTree(bpt, tid);
        NVHTM_end();
        return 1;
    }
    InternalNode *parent;
    // while (1) {
    // _xbegin();
    LeafNode *target_leaf = findLeaf(bpt->root, kv.key, &parent, NULL);
    if (searchInLeaf(target_leaf, kv.key) != -1) {
        // exist
        // _xend();
        NVHTM_end();
        return 0;
    }
    // _xend();
    // }

    if (NVM_read(&target_leaf->key_length) < MAX_PAIR) {
        insertNonfullLeaf(target_leaf, kv);
    } else {
        LeafNode *new_leaf = newLeafNode(tid);
        Key split_key = splitLeaf(target_leaf, kv, tid, new_leaf);
        // _xbegin();
        insertParent(bpt, parent, split_key, new_leaf, target_leaf);
        // _xend();
    }
    NVHTM_end();
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

int findMaxKey(LeafNode *leaf) {
    return NVM_read(&leaf->kv[leaf->key_length - 1].key);
}

// restructuring BPtree from leaf nodes
void insertLeaf(BPTree *emptyTree, LeafNode *leafHead) {
    InternalNode *root = emptyTree->root;
    root->children[0] = leafHead;
    emptyTree->head = leafHead;
    NVHTM_begin();
    LeafNode *target_leaf = getTransientAddr(NVM_read(&leafHead->pnext));
    while(target_leaf != NULL) {
        Key split_key;
        InternalNode *split_node;
        int new_key = findMaxKey(NVM_read(&target_leaf->prev));
        int splitted = insertRecursive(root, new_key, target_leaf, &split_key, &split_node);
        if (splitted) {
            // need to update root
            InternalNode *new_root = newInternalNode();
            new_root->children[0] = emptyTree->root;
            insertNonfullInternal(new_root, split_key, split_node);
            new_root->children_type = INTERNAL;
            emptyTree->root = new_root;
        }
        target_leaf = getTransientAddr(NVM_read(&target_leaf->pnext));
    }
    NVHTM_end();
}

int bptreeUpdate(BPTree *bpt, KeyValuePair new_kv, unsigned char tid) {
    if (bpt == NULL) {
        return 0;
    } else {
        unsigned char split_flag = 0;
        Key split_key = UNUSED_KEY;
        int target_index = -1;
        LeafNode *splitted_leaf = NULL;
        LeafNode *new_leaf = NULL;
        // while (1) {
        // _xstart();
        InternalNode *parent = NULL;
        NVHTM_begin();
        LeafNode *target = findLeaf(bpt->root, new_kv.key, &parent, NULL);
        if (target == NULL) {
            NVHTM_end();
            return 0;
        }
        target_index = searchInLeaf(target, new_kv.key);
        if (target_index == -1) {
            NVHTM_end();
            return 0;
        }
        // _xend();
        // }
        NVM_write_varsize(&target->kv[target_index], &new_kv, sizeof(KeyValuePair));
        //persist(&target->pleaf->kv[target_index], sizeof(KeyValuePair));
        // _xbegin();
        // _xend();
    }
    NVHTM_end();
    return 1;
}

void deleteLeaf(BPTree *bpt, LeafNode *current, unsigned char tid) {
    ppointer pp_tmp;
    LeafNode *leaf_tmp;
    if (NVM_read(&current->prev) == NULL) {
        bpt->head = NVM_read(&current->next);
        *bpt->pmem_head = NVM_read(&current->pnext);
        persist(bpt->pmem_head, sizeof(ppointer));
    } else {
        pp_tmp = NVM_read(&current->pnext);
        NVM_write_varsize(&current->prev->pnext, &pp_tmp, sizeof(ppointer));
        leaf_tmp = NVM_read(&current->next);
        NVM_write_varsize(&current->prev->next, &leaf_tmp, sizeof(LeafNode *));
    }
    if (NVM_read(&current->next) != NULL) {
        leaf_tmp = NVM_read(&current->prev);
        NVM_write_varsize(&current->next->prev, &leaf_tmp, sizeof(LeafNode *));
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

// when removing last node, anchor node's key is not updated.
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
    int i;
    for (i = target_node->key_length; 0 < i; i--) {
        target_node->keys[i + move_length - 1] = target_node->keys[i - 1];
        target_node->children[i + move_length] = target_node->children[i];
    }
    target_node->children[i + move_length] = target_node->children[i];

    target_node->keys[move_length - 1] = *anchor_key;

    for (i = move_length - 1; 0 < i; i--) {
        target_node->keys[i - 1] = left_node->keys[left_node->key_length - move_length + i];
        target_node->children[i] = left_node->children[left_node->key_length - move_length + i + 1];
        left_node->keys[left_node->key_length - move_length + i] = UNUSED_KEY;
        left_node->children[left_node->key_length - move_length + i + 1] = NULL;
    }
    *anchor_key = left_node->keys[left_node->key_length - move_length + i];
    target_node->children[i] = left_node->children[left_node->key_length - move_length + i + 1];
    left_node->keys[left_node->key_length - move_length + i] = UNUSED_KEY;
    left_node->children[left_node->key_length - move_length + i + 1] = NULL;

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
        printf("move:%d to [%d]\n", right_node->keys[i], target_node->key_length + 1 + i);
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
        printf("fill:%d -> keys[%d]\n", target_node->keys[i], i);
        printf("fill:%p -> children[%d]\n", target_node->children[i], i);
#endif
    }
    right_node->keys[i] = *anchor_key;
    right_node->children[i] = target_node->children[i];
#ifdef DEBUG
    printf("fill:%d -> keys[%d]\n", *anchor_key, i);
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
    InternalNode *parent;
    KeyValuePair kv_tmp;
    // while (aborted == true)
    // _xbegin();
    NVHTM_begin();
    LeafNode *target_leaf = findLeaf(bpt->root, target_key, &parent, NULL);
    if (target_leaf == NULL) {
        NVHTM_end();
        return 0;
    }
    int keypos = searchInLeaf(target_leaf, target_key);
    if (keypos == -1) {
#ifdef DEBUG
        printf("delete:the key doesn't exist. abort.\n");
#endif
        NVHTM_end();
        return 0;
    }
    if (NVM_read(&target_leaf->key_length) == 1) {
        deleteLeaf(bpt, target_leaf, tid);
        removeFromParent(bpt, parent, target_leaf, target_key);
    } else {
        // _xend();
        for (int i = keypos; i < NVM_read(&target_leaf->key_length) - 1; i++) {
            kv_tmp = NVM_read(&target_leaf->kv[i + 1]);
            NVM_write_varsize(&target_leaf->kv[i], &kv_tmp, sizeof(KeyValuePair));
        }
        initKeyValuePair(&target_leaf->kv[target_leaf->key_length - 1]);
        target_leaf->key_length--;
    }
    NVHTM_end();
    return 1;
}

/* debug function */
void showLeafNode(LeafNode *node, int depth) {
    int i, j;
    LeafNode *next = NVM_read(&node->next);
    LeafNode *prev = NVM_read(&node->prev);
    int len = NVM_read(&node->key_length);
    ppointer pnext = NVM_read(&node->pnext);
    Key key;
    Value val;

    for (j = 0; j < depth; j++)
        printf("\t");
    printf("leaf:%p\n", node);
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("\tnext = %p, prev = %p, pnext = %p\n", next, prev, getTransientAddr(pnext));
    for (j = 0; j < depth; j++)
        printf("\t");
    printf("\tlength = %d\n", len);
    for (i = 0; i < len; i++) {
        for (j = 0; j < depth; j++)
            printf("\t");
        key = NVM_read(&node->kv[i].key);
        val = NVM_read(&node->kv[i].value);
        printf("\tkv[%d]:key = %ld, val = %d\n", i, key, val);
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
    NVHTM_begin();
    printf("leaf head:%p\n", bpt->head);
    showInternalNode((InternalNode *)bpt->root, 0);
    NVHTM_end();
}
