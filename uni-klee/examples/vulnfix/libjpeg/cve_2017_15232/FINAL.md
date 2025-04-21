```c
// One line 126
METHODDEF(void)
dg_check(JSAMPARRAY buf) {}

// On line 137
dg_check(output_buf);
```

```shell
cp jdmarker.dg.c source/jdmarker.c
cd source
make clean
CC=wllvm CXX=wllvm++ ./configure --without-simd CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g"
make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10
cp source/djpeg ./djpeg
make snapshot
make line-infos.csv
```