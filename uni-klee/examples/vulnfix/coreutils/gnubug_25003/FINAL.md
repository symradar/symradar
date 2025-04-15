```c
// One line 975
static void
dg_check(size_t initial_read) {}

// On line 988
dg_check(initial_read);
```

```shell
cp split.dg.c source/src/split.c
cd source && make CC=wllvm CXX=wllvm++ CFLAGS="-static -O0 -g -fsanitize=address,undefined" CXXFLAGS="-static -O0 -g -fsanitize=address,undefined" -j10
cp source/src/split ./split
make snapshot
make line-infos.csv
```