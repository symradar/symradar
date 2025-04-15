#!/bin/bash
cd /root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/pacfix
make clean && make CFLAGS="-static -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-static -fsanitize=address -fsanitize=undefined -g" -j10
cd /root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633
