#include "min_nvm.h"
#include <string.h>
#include <mutex>
#ifdef USE_PMEM
#include <x86intrin.h>
#endif

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

#ifdef WRITE_AMOUNT_NVHTM
__thread unsigned long written_bytes_thr = 0;
unsigned long written_bytes = 0;
#endif

static std::mutex mtx;

int SPIN_PER_WRITE(int nb_writes)
{
#ifndef USE_PMEM
 	ts_s _ts1_ = rdtscp();
 	SPIN_10NOPS(NH_spins_per_100 * nb_writes);
 	MN_count_spins += nb_writes;
 	MN_time_spins += rdtscp() - _ts1_;
#endif
	return nb_writes;
}

int MN_write(void *addr, void *buf, size_t size, int to_aux)
{
	MN_count_writes++;
#ifndef USE_PMEM
	if (to_aux) {
		// it means it does not support CoW (dynamic mallocs?)
		if (aux_pool == NULL) aux_pool = (int*)malloc(SIZE_AUX_POOL);
		uintptr_t given_addr = (uintptr_t)addr;
		uintptr_t pool_addr = (uintptr_t)aux_pool;
		// place at random within the boundry
		given_addr = given_addr % (SIZE_AUX_POOL - size);
		given_addr = given_addr + pool_addr;
		void *new_addr = (void*)given_addr;
		memcpy(new_addr, buf, size);
		return 0;
	}
#endif

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

void MN_thr_exit()
{
	mtx.lock();
	MN_count_spins_total        += MN_count_spins;
	MN_time_spins_total         += MN_time_spins;
	MN_count_writes_to_PM_total += MN_count_writes;
	mtx.unlock();
#ifdef WRITE_AMOUNT_NVHTM
    fprintf(stderr, "add written_bytes_thr:%lu\n", written_bytes_thr);
    __sync_fetch_and_add(&written_bytes, written_bytes_thr);
#endif
}

void MN_flush(void *addr, size_t size, int do_flush)
{
	int i;
	int size_cl = CACHE_LINE_SIZE / sizeof (char);
	int new_size = size / size_cl;

	// TODO: not cache_align flush

	if (size < size_cl) {
		new_size = 1;
	}

	for (i = 0; i < new_size; i += size_cl) {
#ifdef USE_PMEM
        _mm_clwb(((char *) addr) + i - ((unsigned long)addr % size_cl));
#else
		// TODO: addr may not be aligned
		if (do_flush) {
			ts_s _ts1_ = rdtscp();
			clflush(((char*) addr) + i); // does not need fence
			MN_count_spins++;
			MN_time_spins += rdtscp() - _ts1_;
		}
        else
			SPIN_PER_WRITE(1);
#endif
	}
#ifdef WRITE_AMOUNT_NVHTM
    written_bytes_thr += new_size * size_cl;
#endif
}

void MN_drain()
{
	mfence();
}

void MN_learn_nb_nops() {
 	const char *save_file = "ns_per_10_nops";
 	FILE *fp = fopen(save_file, "r");
 
 	if (fp == NULL) {
#ifndef USE_PMEM
 		// File does not exist
 		unsigned long long ts1, ts2;
 		double time;
 		// in milliseconds (CPU_MAX_FREQ is in kilo)
 		double ns100 = (double)NVM_LATENCY_NS * 1e-6; // moved to 500ns
 		const unsigned long long test = 99999999;
 		unsigned long long i = 0;
 		double measured_cycles = 0;
 
 		// CPU_MAX_FREQ is in kiloHz
 
 		printf("CPU_MAX_FREQ=%llu\n", CPU_MAX_FREQ);
 
#endif
 		fp = fopen(save_file, "w");
#ifndef USE_PMEM
 
 		ts1 = rdtscp();
 		SPIN_10NOPS(test);
 		ts2 = rdtscp();
 
 		measured_cycles = ts2 - ts1;
 
 		time = measured_cycles / (double) CPU_MAX_FREQ; // TODO:
 
 		SPINS_PER_100NS = (double) test * (ns100 / time) + 1; // round up
#endif
 		fprintf(fp, "%i\n", SPINS_PER_100NS);
 		fclose(fp);
#ifndef USE_PMEM
 		printf("measured spins per 100ns: %i\n", SPINS_PER_100NS);
 	} else {
#endif
 		fscanf(fp, "%i\n", &SPINS_PER_100NS);
 		fclose(fp);
 	}
}

void MN_enter() {}
void MN_exit(char is_chkp) {
#ifdef WRITE_AMOUNT_NVHTM
    if (is_chkp) {
        fprintf(stderr, "write amount (checkpoint): %lu\n", written_bytes);
    } else {
        fprintf(stderr, "write amount (worker): %lu\n", written_bytes);
    }
#endif
}
void MN_thr_reset() {
#ifdef WRITE_AMOUNT_NVHTM
    fprintf(stderr, "MN_thr_reset: written_bytes_thr = %lu\n", written_bytes_thr);
    written_bytes_thr = 0;
#endif
}
void MN_start_freq(int by_chkp) {}
