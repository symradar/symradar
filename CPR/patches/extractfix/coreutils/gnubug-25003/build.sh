#!/bin/bash
cd /root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/pacfix
make clean && make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j10
cd /root/projects/CPR/patches/extractfix/coreutils/gnubug-25003