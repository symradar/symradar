#!/bin/bash
CPR_DIR=${CPR_DIR:-/root/projects/CPR}
PATH=$PATH:$CPR_DIR/tools
rm -rf src patched
mkdir -p patched
project_url=https://github.com/vadz/libtiff.git
commit_id=9a72a69
patched_dir=libtiff
patched_file=tif_ojpeg.c
bin_dir=tools
bin_file=tiffmedian
git clone $project_url src
pushd src
  git checkout $commit_id
  wget http://www.ijg.org/files/jpegsrc.v8d.tar.gz
  tar xvzf jpegsrc.v8d.tar.gz
  pushd jpeg-8d
    ./configure --prefix=${PWD}/build
    make -j32 install
  popd
  # Patch
  cp ../${patched_file} ${patched_dir}/${patched_file}
  ./autogen.sh
  LD=lld OJPEG_SUPPORT=true JPEG_SUPPORT=true CC=cpr-cc CXX=cpr-cxx ./configure --enable-static --disable-shared --enable-old-jpeg --with-jpeg-include-dir="${PWD}/jpeg-8d/build/include" --with-jpeg-lib-dir="${PWD}/jpeg-8d/build/lib"
  OJPEG_SUPPORT=true JPEG_SUPPORT=true CC=cpr-cc CXX=cpr-cxx make CFLAGS="-static -O0 -g -fno-discard-value-names" CXXFLAGS="-static -O0 -g -fno-discard-value-names" -j16
  # cp
  cp ${patched_dir}/${patched_file} ../patched
  cp ${bin_dir}/${bin_file} ../patched
popd
pushd patched
  cp ../exploit.tif .
  extract-bc ${bin_file}
  llvm-dis ${bin_file}.bc
popd
