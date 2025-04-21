```c
// One line 1575
static void dg_check(int sampling) {}

// On line 1635
dg_check(sp->v_sampling);
```

```shell
cp tif_jpeg.dg.c ./source/libtiff/tif_jpeg.c

cd source && make CC="wllvm" CXX="wllvm++" CFLGAS="-static -g -O0 -fno-builtin -DJPEG_SUPPORT=true" -j 32
cp source/tools/tiffcp ./
make snapshot
make line-infos.csv
```