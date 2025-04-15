#! /bin/bash

git clone https://github.com/stp/stp.git
cd stp && mkdir build && cd build
cmake ..
make
make install