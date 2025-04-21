CC=wllvm
CFLAGS= -O0 -g -I ../../include

all: div

div: div.bc
	$(CC) $(CFLAGS) -c div.c -o div

div.bc: div.c
	$(CC) $(CFLAGS) -emit-llvm -c div.c
	llvm-dis div.bc

clean:
	rm -rf a.out *.bc *.o *.ll div klee-*

test:
	klee div.bc