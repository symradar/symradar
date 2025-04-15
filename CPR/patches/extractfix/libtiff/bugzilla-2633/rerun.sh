#!/bin/bash
pushd pacfix/tools
  cp tiff2ps.c.i.c tiff2ps.c
popd
rm -rf runtime
mkdir -p runtime/afl-in
mkdir runtime/afl-out

/root/projects/pacfix/main.exe -debug -nouniq -seed -epsilon 0.0 -cycle 60 -timeout 300 ./config
