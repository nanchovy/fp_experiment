include ./Makefile_location.inc
VPATH = $(BUILD_DIR):$(TEST_SRC_DIR)/$(TYPE):$(TEST_SRC_DIR)/$(TREE):$(BASE_BENCH_SRC_DIR):$(HTM_ALG_DIR)/bin:$(MIN_NVM_DIR)/bin

.PRECIOUS: %.o

PMDK_DIR		:= $(HOME)/local
PMDK_INCLUDES	:= -I$(PMDK_DIR)/include/
PMDK_LIBS		:= -L$(PMDK_DIR)/lib -lpmem

include ./Makefile_nvhtm.inc

ifdef no_persist
	NO_PERSIST := -DNPERSIST
else
	NO_PERSIST :=
endif
ifdef nclwb
	CLWB :=
else
	CLWB := -DCLWB
endif
ifeq ($(type), concurrent)
	CONCURRENT := -DCONCURRENT
else
	CONCURRENT :=
endif
ifeq ($(type), nvhtm)
	CONCURRENT	:= -DCONCURRENT
	NVHTM		:= -DNVHTM
	NVHTM_LIB	:= $(BUILD_DIR)/libnh.a
	NVHTM_CLEAN	:= nvhtm-clean
else
	NVHTM :=
	NVHTM_CLEAN	:=
endif
ifeq ($(time), time_part)
	TIME_PART := -DTIME_PART
else
	TIME_PART :=
endif
ifeq ($(tree), bptree)
	TREE_D		:= -DBPTREE
	TREE_OBJ	:= $(BPTREE_SRC:%.c=%.o)
else
	TREE_D		:= -DFPTREE
	TREE_OBJ	:= $(FPTREE_SRC:%.c=%.o)
endif
DEBUGF	:= -g
ifeq ($(debug), 1)
	DEBUG 	:= -DDEBUG
	DEBUGF	:= -O0 -g
endif
ifeq ($(ndebug), 1)
	DEBUG	:= -DNDEBUG
	DEBUGF	:= -O0
	NVHTM_MAKE_ARGS	+= NDEBUG=1
endif
ifeq ($(cw), 1)
	CW	:= -DCOUNT_WRITE
else
	CW	:=
endif
ifeq ($(ca), 1)
	CA	:= -DCOUNT_ABORT
else
	CA	:=
endif
ifeq ($(fw), 1)
	FW		:= -DFREQ_WRITE
	DMN_DIR	:= $(ROOT_DIR)/dummy_min-nvm_wfreq
else
	FW		:=
	DMN_DIR	:= $(ROOT_DIR)/dummy_min-nvm
endif
ifeq ($(ts), 1)
	TS	:= -DTRANSACTION_SIZE
else
	TS	:=
endif
ifdef leafsz
	LEAFSZ	:= -DMAX_PAIR=$(leafsz)
endif
ifeq ($(write_amount),1)
	WA	:= -DWRITE_AMOUNT
else
	WA	:=
endif

DEFINES = $(NVHTM) $(CLWB) $(CONCURRENT) $(NO_PERSIST) $(TIME_PART) $(TREE_D) $(DEBUG) $(CW) $(CA) $(FW) $(TS) $(WA) $(LEAFSZ)

CFLAGS=$(DEBUGF) -march=native -pthread $(DEFINES) -I$(INCLUDE_DIR) $(NVHTM_CFLAGS)

ALLOCATOR_OBJ=$(ALLOCATOR_SRC:%.c=%.o)
THREAD_MANAGER_OBJ=$(THREAD_MANAGER_SRC:%.c=%.o)
RAND_OBJ=$(RAND_SRC:%.c=%.o)

TEST_EXE		:= $(TEST_SRC_NAME:%.c=%.exe)
BASE_BENCH_EXE	:= $(BASE_BENCH_SRC_NAME:%.c=%.exe)
ALL_EXE			:= $(TEST_EXE) $(BASE_BENCH_EXE)
ALL_OBJ			:= $(ALL_EXE:%.exe=%.o)

all: $(ALL_EXE)

include ./Makefile_bench.inc

%.exe:%.o $(TREE_OBJ) $(ALLOCATOR_OBJ) $(THREAD_MANAGER_OBJ) $(RAND_OBJ) $(NVHTM_LIB)
	$(CXX) -o $(BUILD_DIR)/$@ $+ $(NVHTM_LIB) $(CFLAGS)

%.o:%.c $(BUILD_DIR)
	$(CC) -o $@ $(CFLAGS) -c $<

$(NVHTM_LIB): libhtm_sgl.a libminimal_nvm.a
	make -C nvhtm clean
	make -C nvhtm $(NVHTM_MAKE_ARGS)
	mkdir -p $(BUILD_DIR)
	cp nvhtm/libnh.a $(NVHTM_LIB)

libhtm_sgl.a:
	(cd $(NVHTM_DIR)/DEPENDENCIES/htm_alg; ./compile.sh "$(HTM_SGL_FLAG)")

libminimal_nvm.a:
	(cd $(MIN_NVM_DIR); ./compile.sh "$(MIN_NVM_FLAG)")

clean:
	rm -f $(TREE_OBJ) $(ALLOCATOR_OBJ) $(THREAD_MANAGER_OBJ) $(ALL_OBJ)

nvhtm-clean:
	rm -f nvhtm/libnh.a $(NVHTM_LIB) $(MIN_NVM_DIR)/bin/libminimal_nvm.a $(NVHTM_DIR)/DEPENDENCIES/htm_alg/bin/libhtm_sgl.a

dist-clean: clean $(NVHTM_CLEAN)
	rm -f $(addprefix $(BUILD_DIR)/, $(ALL_EXE))

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
