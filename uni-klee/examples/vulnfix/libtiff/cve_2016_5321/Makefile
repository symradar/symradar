TARGET=tiffcrop
TARGET_SOURCE=tiffcrop.c
TARGET_FUNCTION=readSeparateTilesIntoBuffer
CMD=exploit ./tmpout.tif
LOC=993
VAR=dg_check

KLEE_LINK_OPT=
SNAPSHOT_DEFAULT_OPTS=--output-dir=snapshot --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=$(TARGET_FUNCTION) $(KLEE_LINK_OPT)
UNI_KLEE_DEFAULT_OPTS=--write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --no-exit-on-error --simplify-sym-indices --make-lazy --target-function=$(TARGET_FUNCTION) $(KLEE_LINK_OPT)
DG_DEFAULT_OPTS=-cutoff-diverging=false -entry=$(TARGET_FUNCTION) -sc '$(TARGET_SOURCE)\#$(TARGET_FUNCTION)\#$(LOC)\#$(VAR)'
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

test: snapshot
	$(UNI_KLEE) $(UNI_KLEE_DEFAULT_OPTS) --snapshot=snapshot/snapshot-last.json $(TARGET_BC) $(CMD)

clean:
	rm -rf klee-* snapshot line-infos.csv
