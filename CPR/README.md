# CPR Benchmark
Benchmark used in CPR.

## Simple Example

```shell
export PATH=/root/projects/CPR/scripts:$PATH

# 1. Compile patches
sympatch.py compile patches

# 2. Build the target program
symutil.py build 14498 # Or run ./init.sh

# 3. Obtain plausible patches
symradar.py filter 14498

# 4. Run SymRadar and analyze the result
symradar.py snapshot 14498 -p high
symradar.py run 14498 --sym-level=high -p high
symradar.py analyze 14498 -p high

# 5. The output table can be found in:
#    patches/extractfix/libjpeg/CVE-2018-14498/patched/high-*/table_v3.sbsv
```
Click [here](../uni-klee/OUTPUT.md) for how to interpret result.

## Experiment Replication
Use `experiments.py` to run experiments for all subjects in the benchmark in parallel.
This script calls `python3 <script> <cmd> <subject>` for each subject.
```shell
export PATH=/root/projects/CPR/scripts:$PATH

# 1. Compile patches
sympatch.py compile patches

# 2. Build subjects
experiments.py util --extra build

# 3. Run the filter step for all subjects
experiments.py filter

# 4. Run SymRadar for all subjects
experiments.py exp --extra high -s high
experiments.py analyze -s high

# 5. Collect final results (check the ./out directory)
experiments.py final -s high
```

## Script commands and options
The scripts directory contains three main scripts (sympatch.py, symutil.py, symradar.py) and the experiments.py script for parallel execution across multiple subjects.
Ensure the scripts directory is in your PATH: `export PATH=/root/projects/CPR/scripts:$PATH`.

### 1. `sympatch.py`
This script extracts concrete patches from CPR-generated patch files and converts them into a meta-program format. The extraction process is already completed; you only need to run the compile command to process the patches for all subjects found in the specified directory.
```shell
# Usage: sympatch.py <cmd> <patch-dir>
# Example: Compile patches located in the 'patches' directory
sympatch.py compile patches
```
### 2. `symutil.py`
This script provides utility functions for building subjects.
```shell
# Usage: symutil.py <cmd> <subject>
# Example: Build a specific subject
symutil.py build <subject>
```

### 3. `symradar.py`
This is the main script for running the `SymRadar` analysis. It invokes `uni-klee` with the appropriate options and analyzes the results.
```shell
# Usage: symradar.py <cmd> <subject> [options]
symradar.py filter <subject>
symradar.py snapshot <subject> -p high
symradar.py run <subject> --sym-level=high -p high
symradar.py uc <subject> -p uc
symradar.py analyze <subject> -p high
```
Note: For `symutil.py` and `symradar.py`, you can specify a subject using only a unique part of its name (e.g., 14498 will be recognized as CVE-2018-14498).

#### `symradar.py` Commands (`<cmd>`):
- filter: Filters patches using concrete inputs.
- run/rerun: Executes the main SymRadar analysis. run reuses an existing KLEE state snapshot if available, while rerun always starts fresh (deleting any existing snapshot).
- analyze: Analyzes the results (`data.log` in output directory) from a run or rerun execution.
- uc: Enables UC-KLEE mode (details assumed specific to the project).

#### `symradar.py` Options (`[options]`):
- `-p <prefix>`: Prefix for the output directory (used by analyze to find results, potentially by run/rerun implicitly).
- `--snapshot-prefix <prefix>`: Prefix for the snapshot directory (used by run/rerun).
- `--sym-level <level>`: Sets the symbolization level (e.g., none, high). We used high in our experiments.
- `-t <timeout>`: Sets the timeout duration (e.g., 12h for 12 hours).
- `--mode <mode>`: Selects the operational mode (symradar or extractfix).
- `--vulmaster-id <id>`: Enables VulMaster mode using the specified patch ID.

### 4. `experiments.py`
This script automates running commands (like build, filter, analyze) across all subjects (28 by default) in parallel.
```shell
# Usage: experiments.py <cmd> [options]
experiments.py util --extra build
experiments.py filter
experiments.py exp --extra high -s high
experiments.py analyze -s high
experiments.py final -s high
```
#### `experiments.py` Main Options:
- `--extra <value>`: Provides necessary arguments to the underlying script command being called (e.g., build type, sym-level). The required value depends on the specific `<cmd>`.
- `-s <prefix>`: Specifies the directory prefix used for SymRadar outputs and snapshots when calling symradar.py.
- `--mode <mode>`: Selects the mode (symradar or extractfix) for relevant commands.
- `--vulmaster`: Enables VulMaster mode for relevant commands.

#### `experiments.py` Commands (`<cmd>`)
* `util`: Calls `symutil.py`.
  - Values for `--extra`:
    - `build`: Build target program.
    - `extractfix-build`: Build target program for `ExtractFix`.
    - `vulmaster-build`: Build target program for `VulMaster` patches.
    - `vulmaster-extractfix-build`: Build target program for `VulMaster` and use `ExtractFix` mode.

* `filter`: Calls `symradar.py`. The `--extra` option is not needed.

* `run`/`exp`: Calls `symradar.py run` or `symradar.py rerun` for snapshot extraction and symbolic execution. These commands automatically run `snapshot` to extract concrete snapshot if they do not exists. If there is an existing snapshot, `run` reuses the snapshot and `exp` deletes and recreates the snapshot before running symbolic execution.
  - Values for `--extra`:
    - `high`: We used this option, which corresponds to `--sym-level=high`.
    - `none`: This one is used in `--mode=extractfix`. (`--sym-level=none`)

* `uc`: Calls `symradar.py`, run target program in `UC-KLEE` mode.

* `analyze`: Calls `symradar.py`. Analysis usually runs automatically after `run`/`exp`, but this command ensures it's done, especially if a run timed out. Recommended before running `final`.

* `final`: Collects results generated by analyze from the individual subject directories (identified by the `-s <prefix>`) and aggregates them into a single summary file (in the ./out directory).

## Other experiments
### `UC-KLEE`
```shell
export PATH=/root/projects/CPR/scripts:$PATH
sympatch.py compile patches
experiments.py util --extra build
experiments.py filter

# Run with uc
experiments.py uc -s uc
experiments.py analyze -s uc
experiments.py final -s uc
```

### `ExtractFix`
```shell
export PATH=/root/projects/CPR/scripts:$PATH
sympatch.py compile patches
# Build with extractfix-build: this command will remove everything in patched/ dir.
# I recommend you to use separate container for this experiment.
experiments.py util --extra extractfix-build
experiments.py filter
# Run with --mode=extractfix
experiments.py exp --extra none -s extractfix --mode=extractfix
experiments.py analyze -s extractfix --mode=extractfix
experiments.py final -s extractfix --mode=extractfix
```

### `VulMaster`
```shell
export PATH=/root/projects/CPR/scripts:$PATH
sympatch.py compile patches
# Build with vulmaster-build: this command does not affect patched/ dir.
experiments.py util --extra vulmaster-build

# Run with --vulmaster
experiments.py filter --vulmaster
experiments.py exp --extra high -s vulmaster --vulmaster
experiments.py analyze -s vulmaster --vulmaster
experiments.py final -s vulmaster --vulmaster

# UC-KLEE
experiments.py uc -s vulmaster-uc --vulmaster
experiments.py analyze -s vulmaster-uc --vulmaster
experiments.py final -s vulmaster-uc --vulmaster
```

### `Vulmaster` + `ExtractFix`
```shell
export PATH=/root/projects/CPR/scripts:$PATH
sympatch.py compile patches
# Build with vulmaster-build: this command will remove everything in vulmaster-patched/ dir.
experiments.py util --extra vulmaster-extractfix-build

# Run with --vulmaster
experiments.py filter --vulmaster
experiments.py exp --extra none -s vulmaster-extractfix --vulmaster --mode=extractfix
experiments.py analyze -s vulmaster-extractfix --vulmaster --mode=extractfix
experiments.py final -s vulmaster-extractfix --vulmaster --mode=extractfix
```

## Environment Setup

### Environment Setup
Build [`uni-klee`](../uni-klee) first.

```shell
# Install python 3.8
python3 -m pip install -r requirements.txt
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

### lib
```
cd /root/projects/CPR/lib
make
```
Other requirements are pre-built in lib/.

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


