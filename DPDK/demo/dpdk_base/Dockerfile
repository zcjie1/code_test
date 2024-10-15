# DPDK base image

FROM dpdk-env:latest

# copy DPDK source code
COPY ./* /usr/src/dpdk/

# install DPDK
WORKDIR /usr/src/dpdk/
RUN tar -xf custom_dpdk.tar && \
    rm -rf custom_dpdk.tar && \
    cd dpdk-custom && \
    meson setup build && \
    cd build && \
    ninja && \
    meson install && \
    ldconfig

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    