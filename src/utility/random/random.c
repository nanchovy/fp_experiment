#include "random.h"

static int *random_array;
static int random_number_of_thread;
static int random_number_of_initial_element;
static int random_number_of_element;

void random_init(int inielenum, int elenum, int thrnum) {
    int i;
    random_array = (int *)malloc(sizeof(int) * (inielenum + elenum));
    random_number_of_initial_element = inielenum;
    random_number_of_element = elenum;
    random_number_of_thread = thrnum;
    srand(0);
    for (i = 0; i < inielenum + elenum; i++) {
        random_array[i] = rand();
    }
}

void random_destroy() {
    free(random_array);
}

int get_rand_initials(int num) {
    return random_array[num];
}

int get_rand(int num, int tid) {
    return random_array[random_number_of_initial_element + tid * (random_number_of_element / random_number_of_thread) + num];
}
