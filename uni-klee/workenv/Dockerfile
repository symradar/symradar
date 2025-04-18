FROM ubuntu:18.04

RUN apt clean
RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y build-essential curl wget software-properties-common git
# add this for installing latest version of python3.8
RUN add-apt-repository ppa:deadsnakes/ppa
RUN add-apt-repository ppa:avsm/ppa
RUN printf "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-12 main" | tee /etc/apt/sources.list.d/llvm-toolchain-xenial-12.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add
RUN apt update

# install a newer version of cmake, since it is required by z3
RUN DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends wget
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN DEBIAN_FRONTEND=noninteractive apt purge --yes --auto-remove cmake && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"  && \
    apt update && \
    apt-get install --yes --no-install-recommends cmake

# install python3.8, for driver scripts of the project
RUN DEBIAN_FRONTEND=noninteractive apt install -y python3.8
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 10

# install other libraries
RUN DEBIAN_FRONTEND=noninteractive apt install -y git vim python3-pip gdb \
    default-jdk m4 xxd clang flex bison autopoint gperf texinfo libjpeg-dev \
    nasm libass-dev libmp3lame-dev dh-autoreconf libopus-dev \
    libtheora-dev libvorbis-dev rsync python3-dev python-dev opam \
    libtcmalloc-minimal4 libgoogle-perftools-dev ninja-build libncurses5-dev \
    subversion 

# install elfutils
RUN DEBIAN_FRONTEND=noninteractive apt install -y unzip pkg-config zlib1g zlib1g-dev autoconf libtool cmake
WORKDIR /root
RUN curl -o elfutils-0.185.tar.bz2 https://sourceware.org/elfutils/ftp/0.185/elfutils-0.185.tar.bz2
RUN tar -xf elfutils-0.185.tar.bz2
WORKDIR /root/elfutils-0.185/
RUN ./configure --disable-debuginfod --disable-libdebuginfod
RUN make
RUN make install

# RUN python3.8 -m pip install -r requirements.txt
# required for building cvc5 (default python3 is 3.6)
RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install toml pyparsing sbsv numpy matplotlib networkx graphviz pydantic wllvm

# RUN wget https://apt.llvm.org/llvm.sh
# RUN bash llvm.sh 12
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y clang-12 libclang-12-dev llvm-12 llvm-12-dev libllvm12 llvm-12-runtime \
     libgmp-dev libmpfr-dev llvm-dev ncurses-dev libclang-dev lld-12 liblld-12 clang-format-12

RUN update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-12 10
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-12 40
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-12 40

# We need to update every llvm tools to version 12
RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-12 10
RUN update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /usr/bin/llvm-symbolizer-12 40
RUN update-alternatives --install /usr/bin/llc llc /usr/bin/llc-12 40
RUN update-alternatives --install /usr/bin/lld lld /usr/bin/lld-12 40
RUN update-alternatives --install /usr/bin/lli lli /usr/bin/lli-12 40
RUN update-alternatives --install /usr/bin/llvm-ar llvm-ar /usr/bin/llvm-ar-12 40
RUN update-alternatives --install /usr/bin/llvm-as llvm-as /usr/bin/llvm-as-12 40
RUN update-alternatives --install /usr/bin/llvm-bcanalyzer llvm-bcanalyzer /usr/bin/llvm-bcanalyzer-12 40
RUN update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-12 40
RUN update-alternatives --install /usr/bin/llvm-diff llvm-diff /usr/bin/llvm-diff-12 40
RUN update-alternatives --install /usr/bin/llvm-dis llvm-dis /usr/bin/llvm-dis-12 40
RUN update-alternatives --install /usr/bin/llvm-link llvm-link /usr/bin/llvm-link-12 40
RUN update-alternatives --install /usr/bin/llvm-mc llvm-mc /usr/bin/llvm-mc-12 40
RUN update-alternatives --install /usr/bin/llvm-nm llvm-nm /usr/bin/llvm-nm-12 40
RUN update-alternatives --install /usr/bin/llvm-objdump  llvm-objdump  /usr/bin/llvm-objdump-12 40
RUN update-alternatives --install /usr/bin/llvm-profdata llvm-profdata /usr/bin/llvm-profdata-12 40
RUN update-alternatives --install /usr/bin/llvm-ranlib llvm-ranlib /usr/bin/llvm-ranlib-12 40
RUN update-alternatives --install /usr/bin/llvm-rtdyld llvm-rtdyld /usr/bin/llvm-rtdyld-12 40
RUN update-alternatives --install /usr/bin/llvm-size llvm-size /usr/bin/llvm-size-12 40
RUN update-alternatives --install /usr/bin/llvm-tblgen llvm-tblgen /usr/bin/llvm-tblgen-12 40
RUN update-alternatives --install /usr/bin/obj2yaml obj2yaml /usr/bin/obj2yaml-12 40
RUN update-alternatives --install /usr/bin/opt opt /usr/bin/opt-12 40
RUN update-alternatives --install /usr/bin/verify-uselistorder verify-uselistorder /usr/bin/verify-uselistorder-12 40
RUN update-alternatives --install /usr/bin/yaml2obj yaml2obj /usr/bin/yaml2obj-12 40
RUN update-alternatives --install /usr/bin/bugpoint bugpoint /usr/bin/bugpoint-12 40
RUN update-alternatives --install /usr/bin/llvm-dwarfdump llvm-dwarfdump /usr/bin/llvm-dwarfdump-12 40
RUN update-alternatives --install /usr/bin/llvm-extract llvm-extract /usr/bin/llvm-extract-12 40

# fix the libLTO.so and LLVMgold.so
RUN rm /usr/lib/libLTO.so
RUN rm /usr/lib/LLVMgold.so
RUN ln -s /usr/lib/llvm-12/lib/libLTO.so /usr/lib/libLTO.so
RUN ln -s /usr/lib/llvm-12/lib/LLVMgold.so /usr/lib/LLVMgold.so

# fix the llvm include path
RUN rm -rf /usr/include/llvm
RUN rm -rf /usr/include/llvm-c
RUN ln -s /usr/lib/llvm-12/include/llvm /usr/include/llvm
RUN ln -s /usr/lib/llvm-12/include/llvm-c /usr/include/llvm-c

# build the project
COPY . /root/projects/uni-klee/
WORKDIR /root/projects/uni-klee/
RUN git submodule init
RUN git submodule update

WORKDIR /root/projects/uni-klee/

# NOTE: this might be slow
RUN ./build.sh

RUN cp -r /root/projects/uni-klee/thirdparty/CPR /root/projects

WORKDIR /root/projects/uni-klee/build/lib
RUN ar rcs libkleeRuntest.a libkleeRuntest.so.1.0

WORKDIR /root/projects/CPR
RUN ./build.sh

ENV PATH="/home/yuntong/vulnfix/bin:${PATH}"
ENV LLVM_COMPILER=clang
ENV CPR_CC=/root/projects/CPR/tools/cpr-cc
ENV CPR_CXX=/root/projects/CPR/tools/cpr-cxx
ENV PATH=$PATH:/root/projects/uni-klee/scripts:/root/projects/CPR/scripts:/root/projects/CPR/tools
ENV LD_LIBRARY_PATH=/root/projects/CPR/lib:/root/projects/uni-klee/build/lib:$LD_LIBRARY_PATH

WORKDIR /root/projects/uni-klee

ENTRYPOINT /bin/bash
