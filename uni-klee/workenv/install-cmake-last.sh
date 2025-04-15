#! /bin/bash

wget https://github.com/Kitware/CMake/releases/download/v3.24.3/cmake-3.24.3.tar.gz

tar -xvf cmake-3.24.3.tar.gz
cd cmake-3.24.3
./bootstrap
make -DCMAKE_USE_OPENSSL=OFF
make install
cmake --version