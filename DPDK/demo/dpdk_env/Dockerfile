# DPDK docker image 

FROM ubuntu:latest

# system requirements
ARG DEPENDENCIES="\
    gcc-11 \
    pkg-config \
    build-essential \
    python3-pyelftools \
    libnuma-dev \
    meson \
    ninja-build \
    pciutils \
    libarchive13 \
    iproute2 \
    libbpf-dev \
    libssl-dev \
"

RUN apt-get update && \
    apt-get install -y $DEPENDENCIES && \
    rm -rf /var/lib/apt/lists/*

# default command
CMD ["/usr/bin/bash"]
    