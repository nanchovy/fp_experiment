#ifndef LOG_BACKWARD_H_GUARD
#define LOG_BACKWARD_H_GUARD

  /**
   * Checks if any log is at the filtering threshold,
   * if so applies from that point.
   */
	int LOG_checkpoint_backward();

  #ifdef PARALLEL_CHECKPOINT
  int LOG_checkpoint_backward_concurrent_apply(int, NVLog_s *, int[], int[]);
  void *LOG_checkpoint_backward_thread_func(void *);
  typedef struct _CL_BLOCK {
    char bit_map;
  } CL_BLOCK;
  #endif

  // called internally
  int LOG_checkpoint_backward_apply_one();


#endif /* end of include guard: LOG_BACKWARD_H_GUARD */
