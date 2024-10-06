# zcio_docker_image

FROM dpdk-env:latest

# copy DPDK source code
COPY ./* /usr/src/zcio_docker/

# install DPDK
WORKDIR /usr/src/zcio_docker
RUN tar -xf custom_dpdk.tar && \
    rm -rf custom_dpdk.tar && \
    cd dpdk-custom && \
    meson setup build && \
    cd build && \
    ninja && \
    meson install && \
    ldconfig

# compile zcio_docker.c
RUN make static

ENV PATH="$PATH:/usr/local/bin"
ENV PATH="$PATH:/usr/src/zcio_docker/build"

# default command
CMD ["/usr/bin/bash"]