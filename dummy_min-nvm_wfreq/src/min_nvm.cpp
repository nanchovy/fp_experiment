#include "min_nvm.h"
#include <string.h>
#include <mutex>
#include <x86intrin.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#ifndef ALLOC_FN
#define ALLOC_FN(ptr, type, size) \
ptr = (type*) malloc(size * sizeof(type))
#endif

#define SIZE_AUX_POOL 4096*sizeof(int)
static int *aux_pool;

/* #ifdef OLD_ALLOC
#define ALLOC_FN(ptr, type, size) posix_memalign((void **) &ptr, \
CACHE_LINE_SIZE, (size * sizeof(type)) * CACHE_LINE_SIZE)
#else
#define ALLOC_FN(ptr, type, size) ptr = ((type*) aligned_alloc(CACHE_LINE_SIZE, \
(size * sizeof(type)) * CACHE_LINE_SIZE))
#endif */

/*
TODO shm

*/

CL_ALIGN int SPINS_PER_100NS;
long long MN_count_spins_total;
unsigned long long MN_time_spins_total;
long long MN_count_writes_to_PM_total;

__thread CL_ALIGN NH_spin_info_s MN_info;

static std::mutex mtx;
#ifndef FREQ_WRITE_BUFSZ
#  define FREQ_WRITE_BUFSZ 2 * 60 * 60 * 17
#endif
#ifndef FREQ_INTERVAL
#  define FREQ_INTERVAL 1024 * 1024
#endif
unsigned int nvhtm_wrote_size_tmp = 0;
char *nvhtm_freq_write_buf = NULL;
unsigned int *nvhtm_freq_write_buf_index = NULL;
int start_freq_log = 0;

int SPIN_PER_WRITE(int nb_writes)
{
// 	ts_s _ts1_ = rdtscp();
// 	SPIN_10NOPS(NH_spins_per_100 * nb_writes);
// 	MN_count_spins += nb_writes;
// 	MN_time_spins += rdtscp() - _ts1_;
	return nb_writes;
}

void MN_start_freq(int by_chkp) {
    start_freq_log = 1;
    unsigned int indx_tmp = __sync_fetch_and_add(nvhtm_freq_write_buf_index, 17);
    struct timespec tm;
    double time_tmp = 0;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tm);
    time_tmp += tm.tv_nsec;  
    time_tmp /= 1000000000;                   
    time_tmp += tm.tv_sec;      
    if (by_chkp) {
        sprintf(nvhtm_freq_write_buf + indx_tmp, "c%15lf\n", time_tmp);
    } else {
        sprintf(nvhtm_freq_write_buf + indx_tmp, "w%15lf\n", time_tmp);
    }
}

pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;

int MN_write(void *addr, void *buf, size_t size, int to_aux)
{
	MN_count_writes++;
	// if (to_aux) {
	// 	// it means it does not support CoW (dynamic mallocs?)
	// 	if (aux_pool == NULL) aux_pool = (int*)malloc(SIZE_AUX_POOL);
	// 	uintptr_t given_addr = (uintptr_t)addr;
	// 	uintptr_t pool_addr = (uintptr_t)aux_pool;
	// 	// place at random within the boundry
	// 	given_addr = given_addr % (SIZE_AUX_POOL - size);
	// 	given_addr = given_addr + pool_addr;
	// 	void *new_addr = (void*)given_addr;
	// 	memcpy(new_addr, buf, size);
	// 	return 0;
	// }

	memcpy(addr, buf, size);

	return 0;
}

void *MN_alloc(const char *file_name, size_t size)
{
	// TODO: do with mmap
	char *res;
	size_t missing = size % CACHE_LINE_SIZE;

	ALLOC_FN(res, char, size + missing);
	//    res = aligned_alloc(CACHE_LINE_SIZE, size + missing);
	//    res = malloc(size);

	return (void*) res;
}

void MN_free(void *ptr)
{
	// TODO: do with mmap
	free(ptr);
}

void MN_thr_enter()
{
	NH_spins_per_100 = SPINS_PER_100NS;
}

void MN_enter()
{
    int err;
    int fd;

    fd = open("write_freq_buf_index.tmp", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        if (fd == -1) {
            perror("open");
            exit(1);
        }
    }
    err = posix_fallocate(fd, 0, sizeof(int));
    nvhtm_freq_write_buf_index = (unsigned int*)mmap(NULL, sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (nvhtm_freq_write_buf_index == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(fd);
    *nvhtm_freq_write_buf_index = 0;

    fd = open("write_freq.txt", O_RDWR);
    if (fd == -1) {
        if (errno == ENOENT) {
            fd = open("write_freq.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        }
        if (fd == -1) {
            perror("open");
            exit(1);
        }
    }
    err = posix_fallocate(fd, 0, FREQ_WRITE_BUFSZ);
    nvhtm_freq_write_buf = (char *)mmap(NULL, FREQ_WRITE_BUFSZ, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (nvhtm_freq_write_buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    *nvhtm_freq_write_buf= 0;
    close(fd);
}

void MN_thr_exit()
{
	mtx.lock();
	MN_count_spins_total        += MN_count_spins;
	MN_time_spins_total         += MN_time_spins;
	MN_count_writes_to_PM_total += MN_count_writes;
	mtx.unlock();
}

void MN_exit(char is_chkp)
{
    if (!is_chkp) {
        munmap(nvhtm_freq_write_buf, FREQ_WRITE_BUFSZ);
        int fd = open("write_freq.txt", O_RDWR);
        if (fd == -1) {
            perror("MN_exit");
        }
        if (ftruncate(fd, *nvhtm_freq_write_buf_index) == -1) {
            perror("MN_exit");
        }
        close(fd);
        munmap(nvhtm_freq_write_buf_index, sizeof(unsigned int));
    }
}

void MN_flush(void *addr, size_t size, int by_chkp)
{
	int i;
	int size_cl = CACHE_LINE_SIZE / sizeof (char);
	int new_size = size / size_cl;

	// TODO: not cache_align flush

	if (size < size_cl) {
		new_size = 1;
	}

	for (i = 0; i < new_size; i += size_cl) {
		// TODO: addr may not be aligned
		// if (do_flush) {
			// ts_s _ts1_ = rdtscp();
			// clflush(((char*) addr) + i); // does not need fence
            _mm_clwb(((char *) addr) + i - ((unsigned long)addr % size_cl));
			// MN_count_spins++;
			// MN_time_spins += rdtscp() - _ts1_;
		// }
        //  else
			// SPIN_PER_WRITE(1);
	}

    size = new_size * size_cl;
    if (start_freq_log) {
        unsigned int tmp = __sync_fetch_and_add(&nvhtm_wrote_size_tmp, size);
        if (tmp + size > FREQ_INTERVAL) {
            int err = pthread_mutex_trylock(&write_lock);
            if (err != EBUSY) {
                __sync_fetch_and_sub(&nvhtm_wrote_size_tmp, tmp + size);
                unsigned int indx_tmp = __sync_fetch_and_add(nvhtm_freq_write_buf_index, 17);
                if (indx_tmp + 17 <= FREQ_WRITE_BUFSZ) {
                    struct timespec tm;
                    double time_tmp = 0;
                    clock_gettime(CLOCK_MONOTONIC_RAW, &tm);
                    time_tmp += tm.tv_nsec;  
                    time_tmp /= 1000000000;                   
                    time_tmp += tm.tv_sec;      
                    if (by_chkp) {
                        sprintf(nvhtm_freq_write_buf + indx_tmp, "c%15lf\n", time_tmp);
                    } else {
                        sprintf(nvhtm_freq_write_buf + indx_tmp, "w%15lf\n", time_tmp);
                    }
                } else {
                    fprintf(stderr, "freq_write: buffer size over\n");
                }
                pthread_mutex_unlock(&write_lock);
            }
        }
    }
}

void MN_drain()
{
	mfence();
}

void MN_learn_nb_nops() {
 	const char *save_file = "ns_per_10_nops";
 	FILE *fp = fopen(save_file, "r");
 
 	if (fp == NULL) {
// 		// File does not exist
// 		unsigned long long ts1, ts2;
// 		double time;
// 		// in milliseconds (CPU_MAX_FREQ is in kilo)
// 		double ns100 = (double)NVM_LATENCY_NS * 1e-6; // moved to 500ns
// 		const unsigned long long test = 99999999;
// 		unsigned long long i = 0;
// 		double measured_cycles = 0;
// 
// 		// CPU_MAX_FREQ is in kiloHz
// 
// 		printf("CPU_MAX_FREQ=%llu\n", CPU_MAX_FREQ);
// 
 		fp = fopen(save_file, "w");
// 
// 		ts1 = rdtscp();
// 		SPIN_10NOPS(test);
// 		ts2 = rdtscp();
// 
// 		measured_cycles = ts2 - ts1;
// 
// 		time = measured_cycles / (double) CPU_MAX_FREQ; // TODO:
// 
// 		SPINS_PER_100NS = (double) test * (ns100 / time) + 1; // round up
 		fprintf(fp, "%i\n", SPINS_PER_100NS);
 		fclose(fp);
// 		printf("measured spins per 100ns: %i\n", SPINS_PER_100NS);
// 	} else {
 		fscanf(fp, "%i\n", &SPINS_PER_100NS);
 		fclose(fp);
 	}
}
