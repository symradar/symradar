# UNI-KLEE
UNI-KLEE (**UN**der-constra**I**ned [KLEE](https://github.com/klee/klee)) is our implementation of KLEE-based symbolic execution tool, which supports under-constrained symbolic execution and lazy initialization.
It supports concrete snapshot extraction, abstract snapshot construction from concrete snapshot, and patch verification based on UC-SE and lazy initialization. Several optimizations and heuristics are applied to support such features.

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


## Run
Check for scripts in [CPR](../CPR).

For output, check here: [OUTPUT](./OUTPUT.md)
