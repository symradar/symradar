#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
rm -rf dafl-src dafl-patched
mkdir -p dafl-patched
project_url=https://github.com/vadz/libtiff.git
commit_id=f3069a5
patched_dir=tools
patched_file=tiff2ps.c
bin_dir=tools
bin_file=tiff2ps
git clone $project_url dafl-src
pushd concrete
  gcc -c -fpic -L. uni_klee_runtime_dafl.c
  gcc -shared -o libdafl_runtime.so uni_klee_runtime_dafl.o
  mv libdafl_runtime.so ../dafl-src
popd
pushd dafl-src
  git checkout $commit_id
  # Patch
  cp ../${patched_file} ${patched_dir}/${patched_file}
  # Remove longjmp calls
  sed -i '118d;221d' libtiff/tif_jpeg.c
  sed -i '153d;2463d' libtiff/tif_ojpeg.c
  sed -i 's|fabs_cpr|fabs|g' $patched_dir/$patched_file
  ./autogen.sh
  ./configure --enable-static --disable-shared --without-threads --without-lzma
  make CFLAGS="-static -O0 -g -DDAFL_ASSERT -Wno-error -ldafl_runtime -L"${PWD}" -I"${PWD}"/../concrete" CXXFLAGS="-static -O0 -g -DDAFL_ASSERT -Wno-error -ldafl_runtime -L"${PWD}" -I"${PWD}"/../concrete" -j16
  # cp
  cp ${bin_dir}/${bin_file} ../dafl-patched/bin
popd