ROOT_DIR    := .
BUILD_DIR   := $(ROOT_DIR)/build
SRC_DIR     := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include
RESULT_DIR  := $(ROOT_DIR)/res

SIMPLE      := simple
CONCURRENT  := concurrent
NVHTM       := nvhtm

ifndef type
    TYPE    := $(SIMPLE)
else
	TYPE	:= $(type)
endif
ifeq ($(tree), bptree)
	TREE		:= bptree
else
	TREE		:= fptree
endif

TEST_SRC_DIR        := $(SRC_DIR)/test
BENCH_SRC_DIR		:= $(SRC_DIR)/benchmark
BASE_BENCH_SRC_DIR	:= $(BENCH_SRC_DIR)/base_operation
UTILITY_DIR         := $(SRC_DIR)/utility
FPTREE_DIR          := $(SRC_DIR)/fptree
BPTREE_DIR          := $(SRC_DIR)/bptree
ALLOCATOR_DIR       := $(UTILITY_DIR)/allocator
THREAD_MANAGER_DIR  := $(UTILITY_DIR)/thread_manager
RAND_DIR            := $(UTILITY_DIR)/random
BENCH_SCRIPT_DIR    := $(UTILITY_DIR)/benchmark_script

BPTREE_SRC_NAME         := bptree.c
ALLOCATOR_SRC_NAME      := allocator.c
THREAD_MANAGER_SRC_NAME := thread_manager.c
RAND_SRC_NAME           := random.c
BENCH_SCRIPT_SRC_NAME   := base_operation.py
OP_TEST_SRC_NAME		:= $(shell ls $(TEST_SRC_DIR)/$(TYPE))
TREE_TEST_SRC_NAME      := $(shell ls $(TEST_SRC_DIR)/$(TREE))
TEST_SRC_NAME           := $(OP_TEST_SRC_NAME) $(TREE_TEST_SRC_NAME)
BASE_BENCH_SRC_NAME		:= $(shell ls $(BASE_BENCH_SRC_DIR))

FPTREE_SRC          := $(FPTREE_DIR)/$(TYPE)/$(BPTREE_SRC_NAME)
BPTREE_SRC          := $(BPTREE_DIR)/$(TYPE)/$(BPTREE_SRC_NAME)
ALLOCATOR_SRC       := $(ALLOCATOR_DIR)/$(TYPE)/$(ALLOCATOR_SRC_NAME)
THREAD_MANAGER_SRC  := $(THREAD_MANAGER_DIR)/$(TYPE)/$(THREAD_MANAGER_SRC_NAME)
RAND_SRC            := $(RAND_DIR)/$(RAND_SRC_NAME)
BENCH_SCRIPT_SRC    := $(BENCH_SCRIPT_DIR)/$(BENCH_SCRIPT_SRC_NAME)
OP_TEST_SRC 		:= $(addprefix $(TEST_SRC_DIR)/$(TYPE)/, $(OP_TEST_SRC_NAME))
TREE_TEST_SRC 		:= $(addprefix $(TEST_SRC_DIR)/$(TREE)/, $(TREE_TEST_SRC_NAME))
TEST_SRC            := $(OP_TEST_SRC) $(TREE_TEST_SRC)
BASE_BENCH_SRC 		:= $(addprefix $(BASE_BENCH_SRC_DIR)/$(TYPE)/, $(BASE_BENCH_SRC_NAME))

