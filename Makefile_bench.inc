BENCH_DATE          ?= $(shell date "+%Y%m%d-%H%M")
BENCH_DIR           := $(RESULT_DIR)/btrees_$(BENCH_DATE)
SCRIPT_SRC_DIR		:= src/utility/benchmark_script

ifdef warmup
	WARM_UP	:= $(warmup)
else
 	WARM_UP	:= 2500000
endif

ifdef loop
	LOOP_TIMES	:= $(loop)
else
	LOOP_TIMES	:= 2500000
#	LOOP_TIMES	:= 5000000
endif

MAX_VAL	:= $(shell expr $(WARM_UP) + $(LOOP_TIMES))

ifdef logmax
	LOGMAX	:= $(logmax)
else
	LOGMAX	:= 0
endif
ifdef logstep
	LOGSTP	:= $(logstep)
else
	LOGSTP	:= 32768
endif
ifdef logstart
	LOGSTT	:= $(logstart)
else
	LOGSTT	:= 41943328
endif

ifdef thrstart
	THRSTT	:= $(thrstart)
else
	THRSTT	:= 1
endif
ifdef thrend
	THREND	:= $(thrend)
else
	THREND	:= 8
endif

ifdef cpthrstart
	CPTHRSTT	:= $(cpthrstart)
else
	CPTHRSTT	:= 8
endif
ifdef thrend
	CPTHREND	:= $(cpthrend)
else
	CPTHREND	:= 8
endif

ifdef ppath
	PPATH	:= $(ppath)
else
	PPATH	:= .
endif

ifdef vdirpath
	VDPATH  := $(vdirpath)
else
	VDPATH	:= .
endif

ifndef parallel_cp
	parallel_cp := 1
endif

LOGLST	?= $(shell seq 0 $(LOGMAX) | sed -e 's/\(.*\)/$(LOGSTT) * 2^\1 + 288/'| bc -q | tr '\n' ' ')

bench_all:
	make PPATH=$(PPATH) VDPATH=$(VDPATH) tree=bptree type=nvhtm use_mmap
	make PPATH=$(PPATH) VDPATH=$(VDPATH) tree=bptree type=nvhtm parallel_cp
	make PPATH=$(PPATH) VDPATH=$(VDPATH) tree=bptree type=nvhtm parallel_cp_dram_log
	make PPATH=$(PPATH) VDPATH=$(VDPATH) tree=bptree type=nvhtm log_compression
	make PPATH=$(PPATH) VDPATH=$(VDPATH) tree=bptree type=nvhtm optimized_commit

use_mmap:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_use_mmap
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) 1 1 $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

use_mmap_dram_log:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_use_mmap
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) 1 1 $(PPATH)/data $(VDPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

parallel_cp:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_parallel_cp
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

parallel_cp_dram_log:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_parallel_cp
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(VDPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

log_compression:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_parallel_cp
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

log_compression_dram_log:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) build_parallel_cp
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(VDPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

number_of_entries:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) entries=$(entries) build_log_compression
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

measure_cp_parallel_cp:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) measure_cp=1 build_parallel_cp
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

measure_cp_log_compression:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) measure_cp=1 build_log_compression
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

optimized_commit:
	mkdir -p $(BENCH_DIR)/$@
	make tree=$(tree) type=$(type) opt_c=1 build_log_compression
	python3 $(SCRIPT_SRC_DIR)/base_operation.py $(BUILD_DIR) $(WARM_UP) $(LOOP_TIMES) $(THRSTT) $(THREND) $(CPTHRSTT) $(CPTHREND) $(PPATH)/data $(PPATH)/log
	$(SCRIPT_SRC_DIR)/copy_results.sh $(BUILD_DIR) $(BENCH_DIR)/$@

build_use_mmap:
	make tree=$(tree) type=$(type) dist-clean
	make tree=$(tree) type=$(type) logsize=$(LOGSZ) stats=1 -j all

build_parallel_cp:
	make tree=$(tree) type=$(type) dist-clean
	make tree=$(tree) type=$(type) logsize=$(LOGSZ) stats=1 parallel_cp=1 measure_cp=$(measure_cp) -j all

build_log_compression:
	make tree=$(tree) type=$(type) dist-clean
	make tree=$(tree) type=$(type) logsize=$(LOGSZ) stats=1 parallel_cp=1 log_compression=1 measure_cp=$(measure_cp) entries=$(entries) opt_c=$(opt_c) -j all
