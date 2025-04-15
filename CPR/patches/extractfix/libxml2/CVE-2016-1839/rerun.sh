pushd pacfix
  cp dict.c.i.c dict.c
  cp HTMLparser.c.i.c HTMLparser.c
popd

rm -rf runtime
mkdir -p runtime/afl-in
mkdir runtime/afl-out

/home/yuntong/pacfix/main.exe -debug -nouniq -seed -epsilon 0.0 -cycle 60 -timeout 300 ./config



