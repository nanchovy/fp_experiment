#ifdef USE_PMEM
#include <pthread.h>
void *stamp_mapped_file_pointer_g = 0;
int stamp_number_of_thread_counter = 1;
__thread void *stamp_mapped_file_pointer = 0;
unsigned int stamp_length_of_mapped_area_per_thread = 0;
__thread stamp_used_bytes = 0;
__thread pthread_mutex_t tm_malloc_mutex;
#endif
