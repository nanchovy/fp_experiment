#include <map>
#include <unordered_map>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifndef THREAD_NUM_MAX
#  define THREAD_NUM_MAX 16
#endif
#ifndef LOOP_TIMES
#  define LOOP_TIMES 10000000
#endif
unsigned int thread_num_g = 1;
unsigned int num_array_g[LOOP_TIMES];

void *inserting_elements(void *arg) {
    unsigned int tid = *(unsigned int *)arg;
    unsigned int seed = tid;
    int i;
    std::unordered_map<unsigned int, int> map;
    map.reserve(32000);
    for (i = 0; i < LOOP_TIMES; i++) {
        if (i % thread_num_g == tid) {
            auto to_insert = std::make_pair(num_array_g[i], i);
            map.insert(to_insert);
        }
    }
    return NULL;
}

int main() {
    pthread_t tids[THREAD_NUM_MAX];
    unsigned int i, thread_num, args[THREAD_NUM_MAX];
    struct timespec stt, edt;
    for (i = 0; i < LOOP_TIMES; i++) {
        num_array_g[i] = rand();
    }
    for (thread_num = 1; thread_num <= THREAD_NUM_MAX; thread_num *= 2) {
        thread_num_g = thread_num;
        clock_gettime(CLOCK_MONOTONIC_RAW, &stt);
        for (i = 0; i < thread_num; i++) {
            args[i] = i;
            pthread_create(&tids[i], NULL, inserting_elements, &args[i]);
        }
        for (i = 0; i < thread_num; i++) {
            pthread_join(tids[i], NULL);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &edt);

        double time = edt.tv_nsec - stt.tv_nsec;
        time /= 1000000000;
        time += edt.tv_sec - stt.tv_sec;
        printf("%d thread: %lf\n", thread_num, time);
    }
    return 0;
}
