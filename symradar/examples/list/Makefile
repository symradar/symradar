CC=wllvm
CFLAGS= -O0 -g -I ../../include


all: list.bc list_for_precondition.bc

list: list.bc
	$(CC) $(CFLAGS) list.c -o list

list_for_precondition: list_for_precondition.bc
	$(CC) $(CFLAGS) list_for_precondition.c -o list_for_precondition

list.bc: list.c
	# $(CC) $(CFLAGS) -emit-llvm -c list.c -fsanitize=address
	$(CC) $(CFLAGS) -emit-llvm -c list.c
	llvm-dis list.bc

list_for_precondition.bc: list_for_precondition.c
	$(CC) $(CFLAGS) -emit-llvm -c list_for_precondition.c
	llvm-dis list_for_precondition.bc

clean:
	rm -f a.out *.bc *.o *.ll list
	rm -rf klee-* snapshot

clean-klee:
	rm -rf klee-*

snapshot: list.bc
	rm -rf snapshot
	uni-klee --output-dir=snapshot --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --max-memory=0 --lazy-patch -target-function=get_sign --patch-id=0 list.bc 4

test: snapshot
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4

uni: snapshot
	uni-klee --start-from-snapshot --dump-snapshot --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low --patch-id=0 --max-memory=0 --lazy-patch --max-forks-per-phases=64,64,64 list.bc 4

log: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4 > klee-log.log 2>&1

dg: snapshot
	llvm-slicer -cutoff-diverging=false -c 36:x -entry=get_sign snapshot/assembly.ll

show: snapshot
	llvm-to-source snapshot/assembly.ll.sliced list.c

sliced: snapshot
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=get_sign --snapshot=snapshot/snapshot-last.json --symbolize-level=low list.bc 4
