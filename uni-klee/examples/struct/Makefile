CC=clang
CFLAGS= -O0 -g


all: struct

struct: struct.bc
	$(CC) $(CFLAGS) struct.c -o struct

struct.bc: struct.c
	# $(CC) $(CFLAGS) -emit-llvm -c struct.c -fsanitize=address
	$(CC) $(CFLAGS) -emit-llvm -c struct.c
	llvm-dis struct.bc

clean:
	rm -f a.out *.bc *.o *.ll struct
	rm -rf klee-* snapshot

clean-klee:
	rm -rf klee-*

snapshot: struct.bc
	rm -rf snapshot
	uni-klee --output-dir=snapshot --write-smt2s --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=get_sign struct.bc 4

test: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=get_sign --snapshot=snapshot/snapshot-1.json --make-lazy struct.bc 4
