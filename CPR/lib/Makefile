CC=gcc
CFLAGS=-I /root/projects/uni-klee/include # $(KLEE_INCLUDE_PATH)

DEP = uni_klee_runtime.h
SRC = uni_klee_runtime.c
PROXY_C = cpr_proxy.c
PROXY_OBJ = cpr_proxy.o
OBJ_KLEE = $(SRC:%.c=%.o)

all: cpr_runtime.o libcpr_runtime.a libcpr_runtime.so libcpr_runtime.bca libcpr_proxy.so libcpr_proxy.a libcpr_proxy.bca libuni_klee_memory_check.a UniKleeGlobalVariablePass.so ExtractFix.so

%.o: %.c $(DEP)
	wllvm -g -fPIC -O0 -c -o $@ $< $(CFLAGS)

libcpr_proxy.so: $(PROXY_OBJ)
	wllvm -fPIC -shared -o $@ $(PROXY_OBJ)

libcpr_proxy.a: $(PROXY_OBJ)
	llvm-ar rcs $@ $(PROXY_OBJ)

libcpr_runtime.so: $(OBJ_KLEE)
	wllvm -fPIC -shared -o $@ $(OBJ_KLEE)

libcpr_runtime.a: $(OBJ_KLEE)
	llvm-ar rcs $@ $(OBJ_KLEE)

libcpr_runtime.bca: libcpr_runtime.a
	extract-bc libcpr_runtime.a

libcpr_proxy.bca: libcpr_proxy.a
	extract-bc libcpr_proxy.a

libuni_klee_memory_check.a: uni_klee_memory_check.c uni_klee_memory_check.h
	wllvm -c -O0 -g -o uni_klee_memory_check.o uni_klee_memory_check.c
	llvm-ar rcs $@ uni_klee_memory_check.o

UniKleeGlobalVariablePass.so: UniKleeGlobalVariablePass.cpp
	clang++ -fPIC -shared -o UniKleeGlobalVariablePass.so UniKleeGlobalVariablePass.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core`

ExtractFix.so: ExtractFix.cpp MetadataExtractor.cpp MetadataExtractor.h
	clang++ -fPIC -shared -o ExtractFix.so ExtractFix.cpp MetadataExtractor.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core`


clean:
	rm -f *.o
	rm -f *.a
	rm -f libcpr_proxy.bca
	rm -f libcpr_runtime.bca
	rm -f *.so
