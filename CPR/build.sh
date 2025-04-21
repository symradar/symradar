#!/bin/bash
pushd lib
  make
popd
python3 scripts/sympatch.py concrete patches
python3 scripts/sympatch.py meta patches