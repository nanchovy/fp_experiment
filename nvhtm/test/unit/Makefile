ROOT = ../..
include $(ROOT)/Makefile_common.inc
include $(ROOT)/Makefile_options.inc

BUILD_FOLDER := $(ROOT)/build/

INCLUDES := $(COMMON_INC) \
	$(SOLUTION_INC)

SRCS     := $(COMMON_SRC) \
	$(SOLUTION_SRC) \
	$(shell ls -rt -d -1 *.c *.cpp)
#
OBJS     := $(addsuffix .o, $(basename $(SRCS)))

LIB      := libnh.a
APP      := test
APP_SRCS := ./test_runner.cpp
APP_OBJS := $(addsuffix .o,$(basename $(APP_SRCS)))

###########################################
CC       := gcc
CXX      := g++
AR       := ar -rv
CFLAGS   := -g $(OPT) -Wall $(HTM_LIB) $(INCLUDES) $(DEFINES)
CXXFLAGS := -g $(OPT) -std=c++11 $(HTM_LIB) $(INCLUDES) $(DEFINES)
LDFLAGS  := -g -lpthread $(INCLUDES) $(DEFINES) $(COMMON_LIBS) -lcppunit
###########################################

build: $(LIB) $(APP)

$(OBJS): $(SRCS)

$(LIB): $(OBJS)
	$(AR) $@ $^

$(APP): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(LIB) $(APP) $(APP_OBJS)
