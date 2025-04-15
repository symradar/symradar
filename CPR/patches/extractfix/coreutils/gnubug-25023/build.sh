#!/bin/bash
cd /root/projects/CPR/patches/extractfix/coreutils/gnubug-25023/pacfix
sed -i '/extern int __builtin_mul_overflow() ;/d; /extern int __builtin_sub_overflow() ;/d; /extern int __builtin_add_overflow() ;/d' src/pr.c
export FORCE_UNSAFE_CONFIGURE=1 && make clean && CC=clang CXX=clang++ make CFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-Wno-error -fsanitize=address -fsanitize=undefined -g" -j10
cd /root/projects/CPR/patches/extractfix/coreutils/gnubug-25023
