```c
// One line 951
static void dg_check(tsample_t s) {}

// On line 993
dg_check(s);
```

```shell
cp tiffcrop.dg.c source/tools/tiffcrop.c
cd source
make clean
CC=wllvm CXX=wllvm++ ./configure --without-simd CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g"
make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10
cp source/tools/tiffcrop ./
make snapshot
make line-infos.csv
```