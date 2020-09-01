#ifndef CONCURRENT
// #  error "CONCURRENT is not defined!"
#endif
#include "allocator.h"

PAddr PADDR_NULL = { 0, 0 };

#ifdef ALLOCTIME
struct timespec myalloc_start_time = {0, 0};
struct timespec myalloc_finish_time = {0, 0};
long long allocate_time = 0;
long long persist_time = 0;
#endif

#define DEFAULT_NODE_NUM 0 // デフォルトでフリーリストに入っている数
#define NB_NODE_AT_ONCE 1

typedef struct FreeNode {
    struct FreeNode *next;
    void *node;
} FreeNode;

typedef struct MemoryRoot {
    unsigned char global_lock;
    void *global_free_area_head;
    // size_t remaining_amount;
    // FreeNode *global_free_list_head; // for after recovery
    // unsigned char **list_lock;
    // FreeNode ***local_free_list_head_ary; // [i][j] -> スレッドiの持つスレッドj用のフリーリストへのポインタ
    // FreeNode ***local_free_list_tail_ary; // [i][j] -> スレッドiの持つスレッドj用のフリーリスト末尾へのポインタ
} MemoryRoot;




/*
 * 構造：
 * | head_ppointer | root_ppointer | node[0] | ...
 * 新規作成の場合head_ppointer = {0, 0}
 * fidは1から．現在のところ複数ファイルには対応していない．
 * また最初に確保した以上のメモリを確保することもない．
 * nスレッドごとにn * nのフリーリストを持ち，ノードのスレッドidと同じリストに返す
 */

int _mmap_fd;
void *_pmem_mmap_head = NULL;
void *_pmem_user_head = NULL;
size_t _pmem_mmap_size = 0;
size_t _pmem_user_size = 0;
int _number_of_thread = -1;
MemoryRoot *_pmem_memory_root = NULL;
size_t _tree_node_size = 0;
unsigned char _initialized_by_others = 0;



PMemHeader base;
PMemHeader *allocp;
#define NALLOC 128

void initMemoryRoot(MemoryRoot *mr, unsigned char thread_num, void *head, size_t pmem_size, size_t node_size, FreeNode *global_list_head) {
    mr->global_lock = 0;
    mr->global_free_area_head = head;
}
void destroyMemoryRoot(MemoryRoot *mr) {
    // TODO: need to traverse free list
    return 0;
}

int isSamePAddr(PAddr a, PAddr b) {
    return (a.fid == b.fid) && (a.offset == b.offset);
}

ppointer getPersistentAddr(void *addr) {
    ppointer paddr;
    if (addr == NULL) {
        return PADDR_NULL;
    } else {
        paddr.fid = 1;  // for single thread
        paddr.offset = addr - _pmem_mmap_head;
        return paddr;
    }
}

void *getTransientAddr(ppointer paddr) {
    if (isSamePAddr(paddr, PADDR_NULL)) {
        return NULL;
    } else {
        return _pmem_mmap_head + paddr.offset;
    }
}

int initAllocator(void *existing_p, const char *path, size_t pmem_size, unsigned char thread_num) {
    _number_of_thread = thread_num;

    if (existing_p != NULL) {
        _initialized_by_others = 1;
        _pmem_mmap_head = existing_p;
        _pmem_mmap_size = pmem_size;
        _pmem_user_head = _pmem_mmap_head + sizeof(AllocatorHeader);
        _pmem_user_size = pmem_size - sizeof(AllocatorHeader);
        memset(_pmem_mmap_head, 0, _pmem_mmap_size);
        *(PAddr *)_pmem_mmap_head = PADDR_NULL;
        return 0;
    }

    struct stat fi;
    int err;
    _mmap_fd = open(path, O_RDWR);
    if (_mmap_fd == -1) {
        if (errno == ENOENT) {
            AllocatorHeader new_header;
            new_header.node_head = PADDR_NULL;
            _mmap_fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            int written_size = write(_mmap_fd, &new_header, sizeof(AllocatorHeader));
            if (written_size < sizeof(ppointer)) {
                perror("write");
                return -1;
            }
            lseek(_mmap_fd, 0, SEEK_SET);
        }
        if (_mmap_fd == -1) {
            perror("open");
            return -1;
        }
    }

    // extend file
    if (fstat(_mmap_fd, &fi) == -1) {
        perror("fstat");
        return -1;
    }
    if (fi.st_size < pmem_size) {
        err = ftruncate(_mmap_fd, pmem_size);
        if (err != 0) {
            perror("ftruncate");
            return -1;
        }
    }

    _pmem_mmap_head = mmap(NULL, pmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, _mmap_fd, 0);
    if (_pmem_mmap_head == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    _pmem_mmap_size = pmem_size;
    _pmem_user_head = _pmem_mmap_head + sizeof(AllocatorHeader);
    _pmem_user_size = pmem_size - sizeof(AllocatorHeader);
    memset(_pmem_mmap_head, 0, _pmem_mmap_size);


    // karmalloc
    // PMemHeader *base_PMemHeader;
    // base_PMemHeader = (PMemHeader *)_pmem_user_head;
    // // *(PMemHeader *)_pmem_user_head = base;
    // *base_PMemHeader = base;
    // // base.s.ptr = &base;
    // // base.s.size = _pmem_user_size - sizeof(PMemHeader);

    // fd can be closed after mmap
    err = close(_mmap_fd);
    if (err == -1) {
        perror("close");
        return -1;
    }

    return 0;
}

ppointer getHeadPPointer() {
    if (_pmem_user_head == NULL) {
        return PADDR_NULL;
    }

    ppointer *head_pointer = (ppointer *)_pmem_user_head;
    return *head_pointer;
}

ppointer recoverAllocator(ppointer (*getNext)(ppointer)) {
    // TODO: implement recovery
    // find free node and make global free list
    return getHeadPPointer();
};

ppointer *root_allocate(size_t size, size_t node_size) {
#ifdef ALLOCTIME
    clock_gettime(CLOCK_MONOTONIC, &myalloc_start_time);
#endif
    _pmem_memory_root = (MemoryRoot *)vol_mem_allocate(sizeof(MemoryRoot));
    initMemoryRoot(_pmem_memory_root, _number_of_thread, _pmem_user_head + size, _pmem_user_size - size, node_size, NULL);
#ifdef ALLOCTIME
    clock_gettime(CLOCK_MONOTONIC, &myalloc_finish_time);
    myalloc_allocate_time += (myalloc_finish_time.tv_sec - myalloc_start_time.tv_sec) * 1000000000L + (myalloc_finish_time.tv_nsec - myalloc_start_time.tv_nsec);
#endif
    _tree_node_size = node_size;
    ppointer *root_p = (ppointer *)_pmem_user_head;
    if (isSamePAddr(PADDR_NULL, ((AllocatorHeader*)_pmem_mmap_head)->node_head)) {
        ((AllocatorHeader *)_pmem_mmap_head)->node_head = getPersistentAddr(_pmem_user_head);
    }
    return root_p;
}

void root_free(ppointer *root) {
}

ppointer pst_mem_allocate(size_t size, unsigned char tid) {
    void *new_node;

    new_node = karmalloc(size);
    return getPersistentAddr(new_node);
}

void *karmalloc(size_t nbytes) {
    PMemHeader *p, *q;
    unsigned nunits;

    nunits = (nbytes + sizeof(PMemHeader) - 1) / sizeof(PMemHeader) + 1; // number of block this function looking for
    if ((q = allocp) == NULL) {
        // initialization
        base.s.ptr = allocp = q = &base;
        base.s.size = 0;
        
        PMemHeader mem_head;
        mem_head.s.ptr = &base;
        mem_head.s.size = (_pmem_user_size - (_pmem_memory_root->global_free_area_head - _pmem_user_head)) / sizeof(PMemHeader); // user size except memory root
        *(PMemHeader *)_pmem_memory_root = mem_head;
        base.s.ptr = (PMemHeader *)_pmem_memory_root;
        p = base.s.ptr;
        printPMemHeaderinfo(p);
        // p->s.ptr = p;
    }
    for (p = q->s.ptr;; q = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {
            if (p->s.size == nunits) // exactly
                q->s.ptr = p->s.ptr;
            else {
                p->s.size -= nunits;
                printf("remaing p size: %d\n", p->s.size);
                p += p->s.size;
                p->s.size = nunits;
           }
            allocp = q;
            // printfreelist(p);
            return ((char *)(p + 1)); // return only data part (without header)
        }
        if (p == allocp && (p = karmorecore(nunits)) == NULL) {
            // when p returns to start block (in case there is not block which has enough memory)
            // TODO: implement morecore
            return (NULL);
        }
    }
}

void printPMemHeaderinfo(PMemHeader *p) {
    printf("adress: %p, ", p);
    printf("number of units: %d, ", p->s.size);
    printf("next adress: %p\n", p->s.ptr);
}

void printfreelist(PMemHeader *p) {
    PMemHeader *start, *printing;
    start = p;
    printing = p;
    while (1)
    {
        printPMemHeaderinfo(printing);
        printing = printing->s.ptr;
        if (start == printing)
            break;
    }
}

void karfree(void *ap) {
	PMemHeader *p, *q;

    p = (PMemHeader *)ap - 1; // freeing unit's header 
    for (q = allocp; !(p > q && p < q->s.ptr); q = q->s.ptr)
        if (q >= q->s.ptr && (p > q || p < q->s.ptr))
			break;

	if (p + p->s.size == q->s.ptr) {
        // if p is next to q->s.ptr (...|p|nextone|...)
		p->s.size += q->s.ptr->s.size;
		p->s.ptr = q->s.ptr->s.ptr;
	} else {
        // there's some space between p and nextone (...|p|...|nextone|...)
		p->s.ptr = q->s.ptr;
    }

	if (q + q->s.size == p) {
        // if p is next to q (...|q|p|...)
		q->s.size += p->s.size;
		q->s.ptr = p->s.ptr;
	}
    else { // (...|q|...|p|...)
        q->s.ptr = p;
    }
	allocp = q;
}

PMemHeader* karmorecore(u_int32_t nu) {
    // char *cp;
    // PMemHeader *up;
    // int rnu;

    // rnu = NALLOC * ((nu + NALLOC - 1) / NALLOC);
    // cp = sbrk(rnu * sizeof(PMemHeader));
    // if ((long)cp == NULL)
    //     return (NULL);
    // up = (PMemHeader *)cp;
    // up->s.size = rnu;
    // karfree((char *)(up + 1));
    perror("karmalloc");
    // return (allocp);

    
    // PMemHeader np;
    // int nofunits;

    // int err = munmap(_pmem_mmap_head, _pmem_mmap_size);
    // if (err == -1) {
    //     perror("munmap");
    //     return -1;
    // }
    // nofunits = (_pmem_mmap_size - sizeof(PMemHeader)) / sizeof(PMemHeader);
    // _pmem_user_size += _pmem_mmap_size;
    // _pmem_mmap_size *= 2;
    // mmap(_pmem_mmap_head, _pmem_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, _mmap_fd, 0);
    // return allocp;
}

void pst_mem_free(ppointer node, unsigned char node_tid, unsigned char tid) {
    karfree(getTransientAddr(node));
}

void *vol_mem_allocate(size_t size) {
    return malloc(size);
}

void vol_mem_free(void *p) {
    free(p);
}

int destroyAllocator() {
    destroyMemoryRoot(_pmem_memory_root);
    vol_mem_free(_pmem_memory_root);

#ifdef ALLOCTIME
    printf("alloctime:%lld.%09lld\n", myalloc_allocate_time/1000000000L, myalloc_allocate_time%1000000000L);
    printf("persisttime:%lld.%09lld\n", myalloc_persist_time/1000000000L, myalloc_persist_time%1000000000L);
#endif

    if (!_initialized_by_others) {
        int err = munmap(_pmem_mmap_head, _pmem_mmap_size);
        if (err == -1) {
            perror("munmap");
            return -1;
        }
    }
    return 0;
}
