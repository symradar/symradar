```c
// One line 236
static void
dg_check(int num_comps) {}

// On line 327
dg_check(cinfo->num_components);
```

```shell
cp jdmarker.dg.c source/jdmarker.c
cd source
make clean
CC=wllvm CXX=wllvm++ ./configure --without-simd CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g"
make make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10
cp source/djpeg ./djpeg
make snapshot
make line-infos.csv
```