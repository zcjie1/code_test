# DPDK base image

FROM dpdk-env:latest

# copy DPDK source code
COPY ./* /usr/src/dpdk/

# install DPDK
WORKDIR /usr/src/dpdk/
RUN tar -xf custom_dpdk.tar && \
    rm -rf custom_dpdk.tar && \
    cd dpdk && \
    meson -Ddisable_libs=flow_classify -Ddisable_drivers=crypto/openssl build && \
    cd build && \
    ninja -j32 && \
    meson install && \
    ldconfig

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    