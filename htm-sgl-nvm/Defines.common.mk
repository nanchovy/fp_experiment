CC       := gcc
CFLAGS   := -std=c++11 -g -w -pthread -fpermissive # -DNDEBUG=1 -mcpu=power8 -mtune=power8 
CFLAGS   += -O0 # TODO
#CFLAGS   += -I $(LIB)
CPP      := g++
CPPFLAGS := $(CFLAGS)
LD       := g++ -g
LIBS     += -lpthread

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib
SRCS := ../lib/globals.c
