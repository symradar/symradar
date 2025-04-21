FROM ubuntu:22.04
MAINTAINER Ridwan Shariffdeen <ridwan@comp.nus.edu.sg>

RUN mkdir -p /root/projects
COPY . /root/projects/uni-klee

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    autoconf \
    bison \
    build-essential \
    cmake \
    curl \
    emacs \
    flex \
    git \
    google-perftools \
    libboost-all-dev \
    libgoogle-perftools-dev \
    libncurses5-dev \
    ninja-build \
    perl \
    python3 \
    python3-pip \
    software-properties-common \
    subversion \
    unzip \
    zlib1g-dev \
    wget \
    openssh-server

RUN echo 'root:root' | chpasswd
RUN service ssh start
EXPOSE 22
WORKDIR /root
CMD ["/usr/sbin/sshd", "-D"]
