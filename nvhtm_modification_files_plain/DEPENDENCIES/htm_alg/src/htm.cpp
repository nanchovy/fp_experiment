#include "htm_retry_template.h"

#include <cstdlib>
#include <thread>
#include <mutex>

#define LOCK(mtx) \
while (!__sync_bool_compare_and_swap(&mtx, 0, 1)) PAUSE()

#define UNLOCK(mtx) \
mtx = 0; \
__sync_synchronize()

using namespace std;

CL_ALIGN int HTM_SGL_var;
__thread CL_ALIGN HTM_SGL_local_vars_s HTM_SGL_vars;

#ifdef STAT
CL_ALIGN double HTM_nanotime_blocked_total;
__thread CL_ALIGN double HTM_nanotime_blocked;
#endif

static mutex mtx;
static int init_budget = HTM_SGL_INIT_BUDGET;
static int threads;
static int thr_counter;

static __thread int is_record;
static __thread int errors[HTM_NB_ERRORS];
static __thread int tid;

void HTM_init_(int init_budget, int nb_threads)
{
#ifdef STAT
  HTM_nanotime_blocked_total = 0;
#endif
  init_budget = HTM_SGL_INIT_BUDGET;
  threads = nb_threads;
  HTM_SGL_var = 0;
  HTM_INIT();
}

void HTM_exit()
{
#ifdef STAT
  fprintf(stderr, "--- HTM time blocked %lf s!\n", HTM_nanotime_blocked_total);
#endif
  HTM_EXIT();
}

void HTM_thr_init()
{
#ifdef STAT
  HTM_nanotime_blocked = 0;
#endif
  mtx.lock();
  tid = thr_counter++;
  HTM_SGL_tid = tid;
  HTM_THR_INIT();
  mtx.unlock();
}

void HTM_thr_exit()
{
#ifdef STAT
  HTM_nanotime_blocked_total += HTM_nanotime_blocked;
#endif
  mtx.lock();
  --thr_counter;
  HTM_THR_EXIT();
  mtx.unlock();
}

int HTM_get_budget() { return init_budget; }
void HTM_set_budget(int _budget) { init_budget = _budget; }

void HTM_enter_fallback()
{
#ifdef STAT
  struct timespec stt, edt;
	clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif
  // mtx.lock();
  while (!__sync_bool_compare_and_swap(&HTM_SGL_var, 0, 1)) {
    PAUSE();
  }
#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
	double time_tmp = 0;
	time_tmp += (edt.tv_nsec - stt.tv_nsec);
	time_tmp /= 1000000000;
	time_tmp += edt.tv_sec - stt.tv_sec;
	HTM_nanotime_blocked += time_tmp;
#endif
  // HTM_SGL_var = 1;
  // __sync_synchronize();
  errors[HTM_FALLBACK]++;
}

void HTM_exit_fallback()
{
  // __sync_val_compare_and_swap(&HTM_SGL_var, 1, 0);
  HTM_SGL_var = 0;
  __sync_synchronize();
  // mtx.unlock();
}

void HTM_block()
{
#ifdef STAT
  struct timespec stt, edt;
	clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
#endif
  while(HTM_SGL_var == 1) {
    __sync_synchronize();
    PAUSE();
  }
#ifdef STAT
  clock_gettime(CLOCK_MONOTONIC_RAW, &edt);
	double time_tmp = 0;
	time_tmp += (edt.tv_nsec - stt.tv_nsec);
	time_tmp /= 1000000000;
	time_tmp += edt.tv_sec - stt.tv_sec;
	HTM_nanotime_blocked += time_tmp;
#endif

  // mtx.lock();
  // mtx.unlock();
}

void HTM_inc_status_count(int status_code)
{
  if (is_record) {
    HTM_ERROR_INC(status_code, errors);
  }
}

// int HTM_update_budget(int budget, HTM_STATUS_TYPE status)
// {
//   int res = 0;
//   // HTM_inc_status_count(status);
//   res = HTM_UPDATE_BUDGET(budget, status);
//   return res;
// }

int HTM_get_status_count(int status_code, int **accum)
{
  int res = 0;
  res = errors[status_code];
  if (accum != NULL) {
    accum[tid][status_code] = errors[status_code];
  }
  return res;
}

void HTM_reset_status_count()
{
  int i, j;
  for (i = 0; i < HTM_NB_ERRORS; ++i) {
    errors[i] = 0;
  }
}

int HTM_get_nb_threads() { return threads; }
int HTM_get_tid() { return tid; }

void HTM_set_is_record(int is_rec) { is_record = is_rec; }
int HTM_get_is_record() { return is_record; }
