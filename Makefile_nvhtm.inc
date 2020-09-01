ifeq ($(type), nvhtm)
NVHTM_DIR		:= $(ROOT_DIR)/nvhtm
NVM_DEP_DIR		:= $(NVHTM_DIR)/DEPENDENCIES
HTM_ALG_DIR		:= $(NVM_DEP_DIR)/htm_alg
MIN_NVM_DIR		:= $(ROOT_DIR)/nvm-emulation

NVHTM_INCLUDES	:= -I$(NVHTM_DIR)/common/ -I$(MIN_NVM_DIR)/include/\
				   -I$(NVM_DEP_DIR)/arch_dep/include/ -I$(HTM_ALG_DIR)/include/\
				   -I$(NVHTM_DIR)/nvhtm_common/ -I$(NVHTM_DIR)/nvhtm_pc/
NVHTM_DEFINES	:= -DDO_CHECKPOINT=5 -DSOLUTION=4 -DSORT_ALG=5
NVHTM_LIBS		:= -L$(MIN_NVM_DIR)/bin -L$(HTM_ALG_DIR)/bin -lhtm_sgl -lminimal_nvm
NVHTM_MAKE_ARGS	:= SOLUTION=4 DO_CHECKPOINT=5 USE_MIN_NVM=0
ifeq ($(forward), 1)
	NVHTM_MAKE_ARGS += SORT_ALG=4
else
	NVHTM_MAKE_ARGS += SORT_ALG=5
endif
ifeq ($(reduce), 1)
	NVHTM_MAKE_ARGS += REDUCE_CP=1
	NVHTM_DEFINES   += -DREDUCE_CP
endif

ifeq ($(faw_cp), 1)
    NVHTM_MAKE_ARGS += FAW_CHECKPOINT=1
    NVHTM_DEFINES   += -DFAW_CHECKPOINT
endif

ifneq ($(emulator), 1)
    MIN_NVM_FLAG    += -DUSE_PMEM
    NVHTM_MAKE_ARGS += USE_PMEM=1
    NVHTM_DEFINES   += -DUSE_PMEM
endif

ifeq ($(stats),1)
	MIN_NVM_FLAG	+= -DSTAT
	NVHTM_MAKE_ARGS += STAT=1
	NVHTM_DEFINES	+= -DSTAT
	HTM_SGL_FLAG	+= -DCMAKE_CXX_FLAGS=-DSTAT -DCMAKE_C_FLAGS=-DSTAT
endif

ifeq ($(no_empty_loop),1)
    NVHTM_MAKE_ARGS += NO_EMPTY_LOOP_TIME=1
    NVHTM_DEFINES   += -DNO_EMPTY_LOOP_TIME
endif

ifeq ($(write_amount_nvhtm),1)
    NVHTM_MAKE_ARGS += WRITE_AMOUNT_NVHTM=1
    NVHTM_DEFINES   += -DWRITE_AMOUNT_NVHTM
    MIN_NVM_FLAG	+= -DWRITE_AMOUNT_NVHTM
endif

ifeq ($(parallel_cp),1)
    NVHTM_MAKE_ARGS += PARALLEL_CHECKPOINT=1
    NVHTM_DEFINES   += -DPARALLEL_CHECKPOINT
endif

ifeq ($(log_compression),1)
    NVHTM_MAKE_ARGS += LOG_COMPRESSION=1
    NVHTM_DEFINES   += -DLOG_COMPRESSION
endif

ifeq ($(measure_cp),1)
    NVHTM_MAKE_ARGS += MEASURE_PART_CP=1
    NVHTM_DEFINES   += -DMEASURE_PART_CP
endif

ifeq ($(task_dist),1)
    NVHTM_MAKE_ARGS += CHECK_TASK_DISTRIBUTION=1
    NVHTM_DEFINES   += -DCHECK_TASK_DISTRIBUTION
endif

ifeq ($(ldiv),1)
    NVHTM_MAKE_ARGS += LARGE_DIV=1
    NVHTM_DEFINES   += -DLARGE_DIV
endif

ifeq ($(entries),1)
    NVHTM_MAKE_ARGS += NUMBER_OF_ENTRIES=1
    NVHTM_DEFINES   += -DNUMBER_OF_ENTRIES
endif

ifeq ($(opt_c),1)
    NVHTM_MAKE_ARGS += OPT_COMMIT=1
    NVHTM_DEFINES   += -DOPT_COMMIT
endif

ifdef logsize
	LOGSZ	:= $(logsize)
else
	LOGSZ	:= 65824
endif
NVHTM_MAKE_ARGS += LOG_SIZE=$(LOGSZ)

NVHTM_CFLAGS	:= $(NVHTM_DEFINES) $(NVHTM_INCLUDES) $(PMDK_INCLUDES) -lpthread -mrtm $(NVHTM_LIBS) $(PMDK_LIBS)

ifeq ($(db), 1)
	NVHTM_MOD_DIR   := $(ROOT_DIR)/nvhtm_modification_files_double_buffering
else
    NVHTM_MOD_DIR   := $(ROOT_DIR)/nvhtm_modification_files_plain
endif
endif
