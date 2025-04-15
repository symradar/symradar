#!/bin/bash

## TODO: find fix
mkdir -p patched
git clone https://github.com/gdraheim/zziplib.git
mv zziplib source
cd source/
git checkout 3a4ffcd
# git checkout 03de3beabbf570474a9ac05d6dc6b42cdb184cd1
cd docs/
wget https://github.com/LuaDist/libzzip/raw/master/docs/zziplib-manpages.tar
cd ../

cp ../memdisk.c ./zzip/memdisk.c
git add zzip/memdisk.c
git commit -m "Apply patch"

CC=wllvm CXX=wllvm++ ./configure
make CFLAGS="-static -O0 -g" CXXFLAGS="-static -O0 -g" -j10

version_dir="$(uname -s)_$(uname -r)_$(uname -m).d"
# finalize the parameterized config file
sed -i "s/<parameter-dir>/$version_dir/g" ../config

cp $version_dir/bins/unzzipcat-mem ../patched
pushd ../patched
  cp ../exploit .
  cp ../Makefile .
popd
