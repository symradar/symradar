#!/bin/bash
pushd pacfix
  cp split.c.i.c  src/split.c
popd
rm -rf runtime
mkdir -p runtime/afl-in
mkdir runtime/afl-out

/root/projects/pacfix/main.exe -uniklee -debug -lvfile ./live_variables -cycle 60 -timeout 300 ./config 

pushd runtime
  DAFL_DFG_SCORE=/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/sparrow-out/bug/slice_dfg.txt PACFIX_COV_EXE=/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/runtime/split.coverage PACFIX_COV_DIR=/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/repair-out AFL_NO_UI=1 PACFIX_TARGET_LINE=988 PACFIX_VAL_EXE=/root/projects/CPR/patches/extractfix/coreutils/gnubug-25003/runtime/split.valuation /root/projects/DAFL/afl-fuzz -i ./afl-in -o ./out -C -t 2000ms -m none -s h -- ./split.instrumented 
popd