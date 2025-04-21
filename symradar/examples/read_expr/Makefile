CC=wllvm
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
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4

uni: snapshot
	uni-klee --start-from-snapshot --dump-snapshot --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4

log: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4 > klee-log.log 2>&1

dg: snapshot
	llvm-slicer -cutoff-diverging=false -c 36:x -entry=get_sign snapshot/assembly.ll

show: snapshot
	llvm-to-source snapshot/assembly.ll.sliced list.c

sliced: snapshot
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4
