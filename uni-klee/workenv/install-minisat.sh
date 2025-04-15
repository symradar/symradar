#! /bin/bash
git clone https://github.com/stp/minisat.git
cd minisat && mkdir build && cd build
cmake ..
make
make install
cd ../..
