CC=wllvm
CFLAGS= -O0 -g -I ../../include -std=c89
CMD=tree.bc exploit.txt
TARGET_FUNCTION=calculate_balance_factor

all: tree.bc

tree: tree.bc
	$(CC) $(CFLAGS) tree.c -o tree

tree.bc: tree.c
	# $(CC) $(CFLAGS) -emit-llvm -c tree.c -fsanitize=address
	$(CC) $(CFLAGS) -emit-llvm -c tree.c
	$(CC) $(CFLAGS) -o tree tree.bc
	llvm-dis tree.bc

clean:
	rm -f a.out *.bc *.o *.ll tree
	rm -rf klee-* snapshot

clean-klee:
	rm -rf klee-*

snapshot: tree.bc
	rm -rf snapshot
	uni-klee --output-dir=snapshot --write-smt2s --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=$(TARGET_FUNCTION) $(CMD)

test: snapshot
	uni-klee --max-solver-time=60s --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --no-exit-on-error --simplify-sym-indices --target-function=$(TARGET_FUNCTION) --snapshot=snapshot/snapshot-last.json --make-lazy $(CMD)

uni: snapshot
	uni-klee --max-solver-time=60s --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --no-exit-on-error --simplify-sym-indices --target-function=$(TARGET_FUNCTION) --snapshot=snapshot/snapshot-last.json --start-from-snapshot --make-lazy $(CMD)

log: snapshot
	klee-ucse --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=$(TARGET_FUNCTION) --snapshot=snapshot/snapshot-1.json --make-lazy $(CMD) > klee-log.log 2>&1

dg: snapshot
	llvm-slicer -cutoff-diverging=false -c 36:x -entry=$(TARGET_FUNCTION) snapshot/assembly.ll

show: snapshot
	llvm-to-source snapshot/assembly.ll.sliced tree.c

sliced: snapshot
	uni-klee --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --print-trace --no-exit-on-error --simplify-sym-indices --target-function=$(TARGET_FUNCTION) --snapshot=snapshot/snapshot-1.json --make-lazy --slicing-info=line-infos.csv $(CMD)
