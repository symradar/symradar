TARGET=xmllint
TARGET_FUNCTION=xmlDumpElementContent
CMD=--recover exploit
LOC=1181
VAR=content

SNAPSHOT_DEFAULT_OPTS=--output-dir=snapshot --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=$(TARGET_FUNCTION)
UNI_KLEE_DEFAULT_OPTS=--write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --no-exit-on-error --simplify-sym-indices --make-lazy --target-function=$(TARGET_FUNCTION)
DG_DEFAULT_OPTS=-cutoff-diverging=false -c $(LOC):$(VAR)
UNI_KLEE=uni-klee
KLEE_SNAPSHOT=uni-klee
DG=llvm-slicer

TARGET_BC=$(TARGET).bc

$(TARGET_BC): $(TARGET)
	extract-bc $(TARGET)

snapshot: $(TARGET_BC)
	$(KLEE_SNAPSHOT) $(SNAPSHOT_DEFAULT_OPTS) $(TARGET_BC) $(CMD)

line-infos.csv: snapshot
	$(DG) $(DG_DEFAULT_OPTS) snapshot/assembly.ll

dg: line-infos.csv

test: snapshot line-infos.csv
	$(UNI_KLEE) $(UNI_KLEE_DEFAULT_OPTS) --snapshot=snapshot/snapshot-last.json $(TARGET_BC) $(CMD)

clean:
	rm -rf klee-* snapshot line-infos.csv