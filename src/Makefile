BINARY = libsh_heap.a libsh_hash.a libsh_ac_auto.a

all: $(BINARY)


#.SUFFIXES: .o .cpp
.SUFFIXES: .o .c
CXX = g++
CC = gcc

CFLAGS= -g -Wall -DMFP_EPOLL -DCORE_DUMP

%.o:%.cpp
	$(CXX) $(CFLAGS) -c $^ $(INC_ALL)
%.o:%.c
	$(CC) $(CFLAGS) -c $^ $(INC_ALL)

libsh_heap.a: sh_heap.o
	ar q libsh_heap.a sh_heap.o

libsh_hash.a: sh_heap.o sh_hash.o
	ar q libsh_hash.a sh_heap.o sh_hash.o

libsh_ac_auto.a: sh_heap.o sh_ac_auto.o
	ar q libsh_ac_auto.a sh_heap.o sh_ac_auto.o

libsh_strkey_to_int.a: sh_strkey_to_int.o sh_heap.o sh_ac_auto.o
	ar q libsh_strkey_to_int.a sh_strkey_to_int.o sh_heap.o sh_ac_auto.o


clean:
	rm -f ../lib/*
	rm -f *.o *~ $(BINARY)
	find . -depth -name "*.o" -exec rm -f {} \;

install:
	cp *.a ../lib
	cp sh_ac_auto.h sh_hash.h sh_heap.h sh_comm_def.h ../include
