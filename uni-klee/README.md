KLEE Symbolic Virtual Machine
=============================

[![Build Status](https://travis-ci.org/klee/klee.svg?branch=master)](https://travis-ci.org/klee/klee)
[![Coverage](https://codecov.io/gh/klee/klee/branch/master/graph/badge.svg)](https://codecov.io/gh/klee/klee)

`KLEE` is a symbolic virtual machine built on top of the LLVM compiler
infrastructure. Currently, there are two primary components:

  1. The core symbolic virtual machine engine; this is responsible for
     executing LLVM bitcode modules with support for symbolic
     values. This is comprised of the code in lib/.

  2. A POSIX/Linux emulation layer oriented towards supporting uClibc,
     with additional support for making parts of the operating system
     environment symbolic.

Additionally, there is a simple library for replaying computed inputs
on native code (for closed programs). There is also a more complicated
infrastructure for replaying the inputs generated for the POSIX/Linux
emulation layer, which handles running native programs in an
environment that matches a computed test input, including setting up
files, pipes, environment variables, and passing command line
arguments.

For further information, see the [webpage](http://klee.github.io/).

## Build

### 0. Environment
```shell
echo core | sudo tee /proc/sys/kernel/core_pattern
cd /sys/devices/system/cpu
echo performance | sudo tee cpu*/cpufreq/scaling_governor
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
ulimit -s unlimited
```
Disable ASLR, adjust stack size.


### 1. Initialize

Clone this repository
```shell
git clone https://github.com/symradar/uni-klee.git
```

Clone all submodules under `thirdparty`.
```shell
git submodule init
git submodule update
```
In `vscode`, you can set `"git.detectSubmodules": false,` to ignore submodules.

Install required packages.
```shell
bison cmake wget curl flex git libtcmalloc-minimal4 libgoogle-perftools-dev ninja-build libncurses5-dev zlib1g-dev gcc g++ autoconf pkg-install unzip subversion software-properties-common build-essential
```

If submodules are updated, you can update local submodules with this commands:
```shell
git submodule update --init --recursive
git submodule foreach git fetch
git submodule update --recursive --remote
```
If something is wrong, you can reset the submodules.
```shell
git submodule deinit -f .
git submodule update --init
```
Then, rebuild the updated submodules.

### 2. LLVM 12
#### 2.1. Build from source code
```shell
cd thirdparty
git clone -b release/12.x https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir llvm-12
cd llvm-12
cmake -G Ninja -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;compiler-rt;lld;lldb' ../llvm
ninja
ninja install
```
Warning: this requires a lot of memory(>64GB) and disk space(>100GB).

#### 2.2. Using package manager
```shell
wget https://apt.llvm.org/llvm.sh
bash llvm.sh 12
```

```shell
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-12 50
update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-12 50
update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-12 50
update-alternatives --install /usr/bin/llvm-as llvm-as /usr/bin/llvm-as-12 50
update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-12 50
update-alternatives --install /usr/bin/llvm-dis llvm-dis /usr/bin/llvm-dis-12 50
update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-12 50
update-alternatives --install /usr/bin/llvm-link llvm-link /usr/bin/llvm-link-12 50
update-alternatives --install /usr/bin/lld lld /usr/bin/lld-12 50
```

#### 2.3. LLVM 6 - old version (Not recommended)
You can use llvm 6 if you want. You can use same method to the older versions of llvm (llvm 6, 7, 8, 9, 10).

Goto [download page](https://releases.llvm.org/download.html) and download the source code.
```shell
cd thirdparty
wget https://releases.llvm.org/6.0.1/llvm-6.0.1.src.tar.xz
wget https://releases.llvm.org/6.0.1/cfe-6.0.1.src.tar.xz
wget https://releases.llvm.org/6.0.1/compiler-rt-6.0.1.src.tar.xz
wget https://releases.llvm.org/6.0.1/clang-tools-extra-6.0.1.src.tar.xz

tar -xvf llvm-6.0.1.src.tar.xz
tar -xvf cfe-6.0.1.src.tar.xz
tar -xvf compiler-rt-6.0.1.src.tar.xz
tar -xvf clang-tools-extra-6.0.1.src.tar.xz

mv llvm-6.0.1.src llvm-6
mv cfe-6.0.1.src llvm-6/tools/clang
mv clang-tools-extra-6.0.1.src llvm-6/tools/clang/tools/clang-tools-extra
mv compiler-rt-6.0.1.src llvm-6/projects/compiler-rt

rm llvm-6.0.1.src.tar.xz
rm cfe-6.0.1.src.tar.xz
rm compiler-rt-6.0.1.src.tar.xz
rm clang-tools-extra-6.0.1.src.tar.xz
```

Build LLVM
```shell
cd llvm-6
mkdir build
cd build
cmake ..
make -j 32
make install
```

### 3. Minisat

```shell
# git clone https://github.com/stp/minisat.git
cd thirdparty/minisat
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DSTATIC_BINARIES=ON \
 -DBUILD_SHARED_LIBS=OFF \
 ..

make -j 32
make install
cd ../../..
```

### 4. STP

```shell
cd thirdparty/stp
# git clone https://github.com/stp/stp.git
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 -DENABLE_PYTHON_INTERFACE:BOOL=OFF \
 -DBUILD_SHARED_LIBS=OFF \
 -DTUNE_NATIVE:BOOL=ON \
 -DNO_BOOST=ON \
 ..

make -j 32
make install
cd ../../..
```

### 5. Z3

```shell
cd thirdparty/z3
# git clone https://github.com/Z3Prover/z3.git
# git checkout z3-4.8.12
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Release \
 ..

make -j 32
make install
cd ../../..
```

### 6. uclibc and the POSIX environment model

```shell
cd thirdparty/klee-uclibc
# git clone https://github.com/klee/klee-uclibc.git

./configure --make-llvm-lib --with-llvm-config="llvm-config"

make KLEE_CFLAGS="-DKLEE_SYM_PRINTF" -j 32

cd ../..
```

### 7. uni-klee

```shell
mkdir build
cd build

cmake \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
 -DCMAKE_CXX_FLAGS="-fno-rtti" \
 -DENABLE_TCMALLOC=ON \
 -DENABLE_SOLVER_STP=ON \
 -DENABLE_SOLVER_Z3=ON \
 -DENABLE_POSIX_RUNTIME=ON \
 -DENABLE_KLEE_UCLIBC=ON \
 -DKLEE_UCLIBC_PATH="../thirdparty/klee-uclibc" \
 -DENABLE_UNIT_TESTS=OFF \
 -DENABLE_SYSTEM_TESTS=OFF \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
 ..

make -j 32
make install
```
If you want to build with gcc, use following options instead.
```shell
 -DCMAKE_C_COMPILER=gcc \
 -DCMAKE_CXX_COMPILER=g++ \
```

If you want to change the source code, apply `clang-format` before commit.
```shell
python3 scripts/clang-format.py
```

Or you can use this command (slow)
```shell
cd build
make format
```

For optimization of log level, you can give this option.
```shell
 -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
```
You can choose any level among
`TRACE`(default) < `DEBUG` < `INFO` < `WARN` < `ERROR` < `CRITIACAL`.

### 8. Other requirements

#### 8.1. wllvm

```shell
pip install wllvm
export LLVM_COMPILER=clang
```

#### 8.2. Benchmarks

```shell
apt-get update && apt-get install -y  \
    autopoint \
    automake \
    bison \
    flex \
    gettext \
    gperf \
    libass-dev \
    libfreetype6 \
    libfreetype6-dev \
    libjpeg-dev \
    libtool \
    libxml2-dev \
    liblzma-dev \
    nasm \
    pkg-config \
    texinfo \
    yasm \
    xutils-dev \
    libpciaccess-dev \
    libpython2-dev \
    libpython3-dev \
    libx11-dev \
    libxcb-xfixes0-dev \
    libxcb1-dev \
    libxcb-shm0-dev \
    libsdl1.2-dev  \
    libvdpau-dev \
    libnuma-dev
```

zlib
```shell
wget https://zlib.net/zlib-1.3.tar.gz
tar -xzvf zlib-1.3.tar.gz
cd zlib-1.3
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_C_COMPILER=wllvm ..
make -j 32
extract-bc libz.a
cp libz.bca ../..
```

liblzma
```shell
git clone https://github.com/kobolabs/liblzma.git
cd liblzma
git checkout 87b7682ce4b1c849504e2b3641cebaad62aaef87
CC=wllvm CXX=wllvm++ CFLAGS="-O0 -g" ./configure --disable-nls --disable-shared --disable-threads
make -j 32
cd src/liblzma/.libs
extract-bc liblzma.a
cp liblzma.bca ../../../..
```

glibc
```shell
git clone https://sourceware.org/git/glibc.git
cd glibc
git switch release/2.34/master

```

openlibm
```shell
git clone https://github.com/JuliaMath/openlibm.git
cd openlibm
git checkout 12f5ffcc990e16f4120d4bf607185243f5affcb8
```


#### 8.3. Environment variables
```shell
export LLVM_COMPILER=clang
export CPR_CC=/root/projects/CPR/tools/cpr-cc
export CPR_CXX=/root/projects/CPR/tools/cpr-cxx
export PATH=$PATH:/root/projects/uni-klee/scripts:/root/projects/CPR/scripts:/root/projects/CPR/tools
export LD_LIBRARY_PATH=/root/projects/CPR/lib:/root/projects/uni-klee/build/lib:$LD_LIBRARY_PATH
```
Add this to the `~/.bashrc`.


### 9. Working environments

Directory or file name should not include `[`, `]`, `,`, since they may cause error in data log analyzer.

First, Dockerfile.workenv image should be build before start workenv file.

```shell
$ docker build -f Dockerfile.workenv -t dg:{$your-tag} .
```

And run following command for executing working environment container.

```shell
$ docker-compose --env-file ./workenv/.env -f ./workenv/docker-compose.yml up
```

## Uni-KLEE
### Modes
#### Snapshot mode


### Options
#### Required
- `--link-llvm-lib={}`: link with bca file
- `snapshot={}`: snapshot file from crashed state
- `--patch-id={}`: patch ids
- `--libc=uclibc`
- `--allocate-determ`
- `--posix-runtime`
- `--external-calls=all`
- `--symplify-sym-indices`
- `--target-function={}`: name of the target function

#### Required in verification mode
These options should not be used in snapshot mode.
- `--no-exit-on-error`
- `--dump-snapshot`
- `--make-lazy`: if not given, `Uni-KLEE` will not make memory symbolic
- `--symbolic-jump-policy={}`: determine jump policy for optimization.

#### Optional
- `--output-dir={}`
- `--simplify-sym-indices`

#### Optional in verification mode
- `--start-from-snapshot`: starts directly from given snapshot
- `--make-all-parameter-symbolic`: make all parameter of target function symbolic

### Example
Snapshot mode
```shell
uni-klee --link-llvm-lib=/root/projects/CPR/patches/extractfix/libtiff/CVE-2016-5321/concrete/libuni_klee_runtime.bca --link-llvm-lib=/root/projects/CPR/lib/libjpeg-8.4.bca --output-dir=debug --write-smt2s --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --target-function=readSeparateTilesIntoBuffer --patch-id=0 tiffcrop.bc exploit.tif out.tiff
```

Verification mode
```shell
uni-klee --link-llvm-lib=/root/projects/CPR/patches/extractfix/libtiff/CVE-2016-5321/concrete/libuni_klee_runtime.bca --link-llvm-lib=/root/projects/CPR/lib/libjpeg-8.4.bca --snapshot=debug/snapshot-last.json --start-from-snapshot --write-smt2s --write-kqueries --patch-id=0,1,2,3,4,5 --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --no-exit-on-error --dump-snapshot --log-trace --print-trace --simplify-sym-indices --target-function=readSeparateTilesIntoBuffer --make-lazy --make-all-parameter-symbolic tiffcrop.bc exploit.tif out.tiff
```

## Experiments
```shell
libtool
```
