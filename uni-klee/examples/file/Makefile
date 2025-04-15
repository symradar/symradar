CC=clang
CFLAGS= -O0 -g -I ../../include


all: list.bc

list: list.bc
	$(CC) $(CFLAGS) list.c -o list

list.bc: list.c
	# $(CC) $(CFLAGS) -emit-llvm -c list.c -fsanitize=address
	$(CC) $(CFLAGS) -emit-llvm -c list.c
	llvm-dis list.bc

clean:
	rm -f a.out *.bc *.o *.ll list
	rm -rf klee-* snapshot

clean-klee:
	rm -rf klee-*

snapshot: list.bc
	rm -rf snapshot
	uni-klee --output-dir=snapshot --write-smt2s --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=get_sign list.bc 4

test: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-1.json --make-lazy list.bc 4

uni: snapshot
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-1.json --make-lazy list.bc 4

log: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-1.json --make-lazy list.bc 4 > klee-log.log 2>&1