```c
// One line 428
static void dg_check(int length) {}

// On line 483 
dg_check(col_sep_length);
```

```shell
cp bitmap_io.dg.c ./source/src/bitmap_io.c
cd source && make CC="wllvm" CXX="wllvm++" CFLAGS="-static -O0 -g -fno-builtin" CXXFLAGS="-static -O0 -g -fno-builtin" -j10
cp source/src/potrace ./
make snapshot
make line-infos.csv
```