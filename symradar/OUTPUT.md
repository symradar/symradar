# UNI-KLEE Output
## Directory structure
```shell
cd /root/projects/CPR
# Compile patches
sympatch.py single /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/concrete
# Build target programs
symfeas.py build 14498
# Run filter
symvass.py filter 14498
# Run experiment
symvass.py rerun 14498 --sym-level=high -p high
```

You can see this output:
```
patched
|-- cjpeg
|-- cjpeg.bc
|-- exploit.bmp
|-- filter
|-- high-0
|-- snapshot-high
```

## Raw data
In `patched/high-0`:
```
patched/high-0
|-- assembly.ll
|-- base-mem.graph
|-- base-mem.symbolic-globals
|-- bb-trace.log
|-- cfg.sbsv
|-- concrete.log
|-- data.log
|-- expr.log
|-- fork-graph
|-- fork-graph.pdf
|-- fork-graph.png
|-- info
|-- memory.log
|-- messages.txt
|-- ppc.log
|-- run.istats
|-- run.stats
|-- table_v3.sbsv
|-- test000001.input
|-- test000001.kquery
|-- test000001.ktest
|-- test000001.mem
|-- test000001.ptr.err
|-- test000001.smt2
...
|-- test000022.input
|-- test000022.kquery
|-- test000022.ktest
|-- test000022.mem
|-- test000022.ptr.err
|-- test000022.smt2
|-- trace.log
|-- uni-klee.debug.log
|-- uni-klee.info.log
|-- uni-klee.trace.log
|-- uni-klee.warn.log
`-- warnings.txt
```

The raw data is in `data.log` file.
`uni-klee.*.log` files are log file, and `test*.*` files correspond to `state` in `UNI-KLEE`.


## After Analysis
`symradar.py` parse and analyze the raw data and generate `table_v3.sbsv`.
```log
[stat] [states] [original 12] [independent 504]
[sym-in] [id 1] [base 3] [test 8] [cnt 90] [patches [1, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 176, 197, 198]]
[sym-in] [id 2] [base 4] [test 13] [cnt 90] [patches [1, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 176, 197, 198]]
[sym-in] [id 3] [base 1] [test 19] [cnt 15] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[remove] [crash] [id 1] [base 3] [test 8] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:157:17:71433] [exit-res ptr.err] [cnt 90] [patches [1, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 176, 197, 198]]
[remove] [crash] [id 2] [base 4] [test 13] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:157:17:71433] [exit-res ptr.err] [cnt 90] [patches [1, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 176, 197, 198]]
[remain] [crash] [id 3] [base 1] [test 19] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:155:17:71411] [exit-res ptr.err] [cnt 15] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[strict] [id 1] [base 3] [test 8] [cnt 44] [patches [1, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 198]]
[strict] [id 2] [base 4] [test 13] [cnt 44] [patches [1, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 198]]
[strict] [id 3] [base 1] [test 19] [cnt 12] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
[strict-remove] [crash] [id 1] [base 3] [test 8] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:157:17:71433] [exit-res ptr.err] [cnt 44] [patches [1, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 198]]
[strict-remove] [crash] [id 2] [base 4] [test 13] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:157:17:71433] [exit-res ptr.err] [cnt 44] [patches [1, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 198]]
[strict-remain] [crash] [id 3] [base 1] [test 19] [exit-loc /root/projects/CPR/patches/extractfix/libjpeg/CVE-2018-14498/src/rdbmp.c:155:17:71411] [exit-res ptr.err] [cnt 12] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
[sym-out] [default] [inputs 3] [cnt 15] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[sym-out] [remove-crash] [inputs 1] [cnt 15] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[sym-out] [strict] [inputs 3] [cnt 12] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
[sym-out] [strict-remove-crash] [inputs 1] [cnt 12] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
[meta-data] [default] [correct 197] [all-patches 90] [sym-input 3] [is-correct True] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[meta-data] [remove-crash] [correct 197] [all-patches 90] [sym-input 1] [is-correct True] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]
[meta-data] [strict] [correct 197] [all-patches 90] [sym-input 3] [is-correct False] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
[meta-data] [strict-remove-crash] [correct 197] [all-patches 90] [sym-input 1] [is-correct False] [patches [78, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132]]
```

`[meta-data] [remove-crash] [correct 197] [all-patches 90] [sym-input 1] [is-correct True] [patches [46, 78, 111, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 197]]`
This line shows result patches. There are 15 patches remaining, including correct patch 197.
There were 3 symbolic inputs (`[sym-in]`), but 2 were removed (`[removed]`) and 1 remained (`[remain]`) by our heuristic.

- `[sym-in]`, `[remove]`, `[remain]`
These lines represent symbolic input.
For exach line, `id` means symbolic input id (you can ignore this), `base` means state id with original program, `test` means state id with patched programs. `cnt` means how many patches remained by this input, and `patches` shows the list of remaining patches.
- `[sym-in]`: Generated symbolic inputs
- `[remove]`: Removed symbolic inputs by heuristic
- `[remain]`: Remaining symbolic input after applying heuristic

- `[meta-data]`: Final result:
`correct` means correct patch id, `all-patches` means number of all patches, `sym-inputs` means all symbolic inputs used, and `is-correct` means remaining patches include correct patch.