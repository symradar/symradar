Patch:
https://github.com/libming/libming/commit/19e7127e29122be571c87bfb90bca9581417d220

PoC:
https://blogs.gentoo.org/ago/2016/11/07/libming-listmp3-global-buffer-overflow-in-printmp3headers-listmp3-c/
https://github.com/asarubbo/poc/blob/master/00034-libming-globaloverflow-printMP3Headers

Command:
> cd /root/source/util
> ./listmp3 /root/exploit

# Build

change make command to 
```shell
make CC="wllvm" CFLAGS="-static -g -O0" CXXFLAGS="-static -g -O0"
```

# klee

```shell
extract-bc listmp3
klee listmp3.bc
```

# llvm-slicer

```shell
llvm-slicer -cutoff-diverging=false -c 105:samplerate_idx assembly.ll &> out.txt
```