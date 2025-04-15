#!/bin/bash
unzip source.zip
cd source/

autoreconf -i
./configure
make CC=wllvm CXX=wllvm++ CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

cp src/appl/imginfo ../
