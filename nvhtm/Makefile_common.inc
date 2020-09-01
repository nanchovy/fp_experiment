LIB_PMEM_PATH ?= ~/local/pmdk
LIB_MIN_NVM_PATH ?= ../nvm-emulation
ARCH_DEP_PATH ?= ./DEPENDENCIES/arch_dep
HTM_ALG_DEP_PATH ?= ./DEPENDENCIES/htm_alg

CPU_MAX_FREQ=$(shell cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq)
MAX_PHYS_THRS=$(shell cat /proc/cpuinfo | grep processor | wc -l)

DEFINES += -DCPU_MAX_FREQ=$(CPU_MAX_FREQ) \
-DMAX_PHYS_THRS=$(MAX_PHYS_THRS)

COMMON_SRC:=$(shell ls -rt -d -1 $(ROOT)/common/src/*.c $(ROOT)/common/src/*.cpp)
COMMON_INC:=-I $(ROOT)/common/ \
-I $(LIB_PMEM_PATH)/include \
-I $(LIB_MIN_NVM_PATH)/include \
-I $(ARCH_DEP_PATH)/include \
-I $(HTM_ALG_DEP_PATH)/include \
#
COMMON_LIBS:=-L $(LIB_PMEM_PATH)/lib -l pmem \
-L $(LIB_MIN_NVM_PATH)/bin -l minimal_nvm \
-L $(HTM_ALG_DEP_PATH)/bin -l htm_sgl \
# -ltcmalloc \
#
