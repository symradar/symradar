TARGET=bsdtar
TARGET_SOURCE=archive_read_support_format_iso9660.c
TARGET_FUNCTION=choose_volume
CMD=-tf ./libarchive-signed-int-overflow.iso
LOC=1096
VAR=dg_check

SNAPSHOT_DEFAULT_OPTS=--output-dir=snapshot --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function=$(TARGET_FUNCTION)
UNI_KLEE_DEFAULT_OPTS=--write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --no-exit-on-error --simplify-sym-indices --make-lazy --target-function=$(TARGET_FUNCTION)
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
	$(DG) $(DG_DEFAULT_OPTS) snapshot/assembly.ll -dont-verify=false

dg: line-infos.csv

test: snapshot line-infos.csv
	$(UNI_KLEE) $(UNI_KLEE_DEFAULT_OPTS) --snapshot=snapshot/snapshot-1.json $(TARGET_BC) $(CMD)

clean:
	rm -rf klee-* snapshot line-infos.csv
