#!/bin/bash

git clone https://github.com/vadz/libtiff.git
mv libtiff source
cd source/
git checkout f3069a5

sed -i '118d;221d' libtiff/tif_jpeg.c
sed -i '153d;2463d' libtiff/tif_ojpeg.c
git add libtiff/tif_ojpeg.c libtiff/tif_jpeg.c
git commit -m 'Remove longjmp calls'

./configure
make CFLAGS="-static -fsanitize=address -fsanitize=undefined -g" CXXFLAGS="-static -fsanitize=address -fsanitize=undefined -g" -j10

cp tools/tiff2ps ../
