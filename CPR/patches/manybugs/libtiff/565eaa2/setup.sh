project_name=libtiff
bug_id=565eaa2
dir_name=$1/manybugs/$project_name/$bug_id
download_url=https://repairbenchmarks.cs.umass.edu/ManyBugs/scenarios/libtiff-bug-2008-12-30-362dee5-565eaa2.tar
current_dir=$PWD
mkdir -p $dir_name
cd $dir_name
#wget $download_url
cp $current_dir/libtiff-bug-2008-12-30-362dee5-565eaa2.tar .
tar xf libtiff-bug-2008-12-30-362dee5-565eaa2.tar
mv libtiff-bug-2008-12-30-362dee5-565eaa2 src
rm libtiff-bug-2008-12-30-362dee5-565eaa2.tar
mv src/* .
rm -rf src
rm -rf  coverage* \
        configuration-oracle \
        local-root \
        limit* \
        *.cache \
        *.debug.* \
        sanity \
        compile.pl \
        *~ \
        test \
        reconfigure \
        preprocessed \
        fixed-program.txt
mv bugged-program.txt manifest.txt
mv *.lines bug-info
mv fix-failures bug-info
mv libtiff src
cd $dir_name/src
cp $current_dir/tif_dirwrite.c ./libtiff/tif_dirwrite.c
cp $current_dir/thumbnail.c ./tools/thumbnail.c
make distclean
chown -R root $dir_name

## Compile libtiff.
make clean
CC=wllvm CXX=wllvm++ ./configure CFLAGS='-g -O0' --enable-static --disable-shared
sed -i '978 s/./\t&/' test/Makefile
CC=wllvm CXX=wllvm++ make CFLAGS="-march=x86-64" -j32

cd $dir_name

## fix the test harness and the configuration script
sed -i "s#/root/mountpoint-genprog/genprog-many-bugs/libtiff-bug-2008-12-30-362dee5-565eaa2#/root/projects/CPR/data/manybugs/libtiff/565eaa2#g" test.sh
sed -i "s#/root/projects/CPR/data/manybugs/libtiff/565eaa2/src/limit#timeout 5#g" test.sh
sed -i "s#/usr/bin/perl#perl#g" test.sh
sed -i "s#cd libtiff#cd src#g" test.sh

cd src

### Prepare for KLEE
## Fix fabs calls (not supported by KLEE).
#sed -i 's/fabs/fabs_cpr/g' libtiff/tif_luv.c
#sed -i 's/fabs/fabs_cpr/g' tools/tiff2ps.c
##sed -i 's/fabs_cpr/fabs/g' libtiff/tif_luv.c
##sed -i 's/fabs_cpr/fabs/g' tools/tiff2ps.c
#
#make CC=$CPR_CC CXX=$CPR_CXX -j32
#
#cd $dir_name
#
##Instrument driver and libtiff
#sed -i '33i // KLEE' src/libtiff/tif_dirwrite.c
#sed -i '34i #include <klee/klee.h>' src/libtiff/tif_dirwrite.c
#sed -i '35i #ifndef CPR_OUTPUT' src/libtiff/tif_dirwrite.c
#sed -i '36i #define CPR_OUTPUT(id, typestr, value) value' src/libtiff/tif_dirwrite.c
#sed -i '37i #endif' src/libtiff/tif_dirwrite.c
##
#sed -i '351i \\tif (__cpr_choice("978", "bool", (int[]){tif->tif_rawcc, orig_rawcc}, (char*[]){"x", "y"}, 2, (int*[]){}, (char*[]){}, 0)' src/libtiff/tif_dirwrite.c
#sed -i '352d' src/libtiff/tif_dirwrite.c
##
#sed -i '359i \\tklee_print_expr("tif->tif_rawcc=", tif->tif_rawcc);' src/libtiff/tif_dirwrite.c
#sed -i '360i \\tCPR_OUTPUT("obs", "i32", tif->tif_rawcc);' src/libtiff/tif_dirwrite.c
#sed -i '361i \\tklee_assert(tif->tif_rawcc == 0);' src/libtiff/tif_dirwrite.c
#
### Compile instrumentation and test driver.
#cd src
#make CXX=$CPR_CXX CC=$CPR_CC CFLAGS="-lcpr_proxy -L/root/projects/CPR/lib -L/root/projects/uni-klee/build/lib  -lkleeRuntest -I/root/projects/uni-klee/include -g -O0" -j32
#cd tools
#extract-bc tiffcp
#
### Copy remaining files to run CPR.
#cd $current_dir
#cp repair.conf $dir_name
#cp spec.smt2 $dir_name
#cp test-input $dir_name
#cp -rf components $dir_name
#cp -rf test-input-files $dir_name
#cp -rf test-expected-output $dir_name
#cp -rf seed-dir $dir_name
#
#### Test with KLEE
##cd /root/projects/CPR/data/manybugs/libtiff/865f7b2/src/tools
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s tiffcp.bc /root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files/22-44-54-64-74-fail-palette-1c-1b.tiff test.tif
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s tiffcp.bc /root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files/13-14-15-16-17-22-43-53-63-73-fail-miniswhite-1c-1b.tiff test.tif
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s tiffcp.bc /root/projects/CPR/data/manybugs/libtiff/865f7b2/seed-dir/2-pass-long_test.tiff test.tif
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s tiffcp.bc /root/projects/CPR/data/manybugs/libtiff/865f7b2/seed-dir/22-40-50-60-70-pass-minisblack-1c-16b.tiff test.tif
###
##cd /root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files
##gen-bout --sym-file "/root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files/22-44-54-64-74-fail-palette-1c-1b.tiff"
##cd /root/projects/CPR/data/manybugs/libtiff/865f7b2/src/tools
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s --seed-out=/root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files/file.bout --allow-seed-extension --resolve-path --named-seed-matching tiffcp.bc A --sym-files 1 3312 test.tif
##klee --posix-runtime --libc=uclibc --link-llvm-lib=/root/projects/CPR/lib/libcpr_runtime.bca --write-smt2s --seed-out=/root/projects/CPR/data/manybugs/libtiff/865f7b2/test-input-files/file.bout --allow-seed-extension --resolve-path --named-seed-matching tiffcp.bc A --sym-files 1 3312 test.tif
###
#
