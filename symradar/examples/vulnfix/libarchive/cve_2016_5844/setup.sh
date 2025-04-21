#!/bin/bash

mkdir -p patched
# download libarchive source (v3.2.0)
wget https://libarchive.org/downloads/libarchive-3.2.0.zip
unzip libarchive-3.2.0.zip
rm libarchive-3.2.0.zip
mv libarchive-3.2.0 source

# compile bsdtar
#   w/o OPENSSL : type inconsistency introduced around v1.1.0
#   w/  UBSAN   : to check exploit
cd source/

# Apply patch
cp ../archive_read_support_format_iso9660.dg.c libarchive/archive_read_support_format_iso9660.c

CC=wllvm CXX=wllvm++ ./configure --without-openssl
# do not include other ubsan to avoid a NULL error which is always caught
make CFLAGS="-O0 -static -g" CXXFLAGS="-O0 -static -g" -j10


cp ./bsdtar ../patched
pushd ../patched
  cp ../Makefile .
  cp ../libarchive-signed-int-overflow.iso .
  cp ../normal.iso .
popd