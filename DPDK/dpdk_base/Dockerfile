# DPDK docker image 

FROM ubuntu:latest

# copy DPDK source code
COPY ./dpdk_base/dpdk.tar.xz /usr/src/

# system requirements
ARG DEPENDENCIES="\
    pkg-config \
    build-essential \
    python3-pyelftools \
    libnuma-dev \
    meson \
    ninja-build \
    pciutils \
    libarchive13 \
    iproute2 \
"

RUN apt-get update && \
    apt-get install -y $DEPENDENCIES && \
    rm -rf /var/lib/apt/lists/*

# install DPDK
WORKDIR /usr/src/
RUN tar -xf dpdk.tar.xz && \
    mv dpdk-stable-23.11.1 dpdk && \
    rm dpdk.tar.xz && \
    cd dpdk && \
    meson setup build && \
    cd build && \
    ninja && \
    meson install && \
    ldconfig

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    