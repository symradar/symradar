```c
// One line 166
void dg_check(struct zzip_file_header *header) {}

// On line 182
dg_check(header);
```

```shell
cp memdisk.dg.c source/zzip/memdisk.c
cd source
CC=wllvm CXX=wllvm++ ./configure --without-simd CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g"
make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10
cp source/djpeg ./djpeg
make snapshot
make line-infos.csv
```