# DPDK base image

FROM dpdk-base:latest

# copy DPDK source code
COPY ./* /usr/src/

# install DPDK
WORKDIR /usr/src/
RUN tar -xf dperf.tar && \
    rm -rf dperf.tar && \
    cd dperf-custom && \
    make clean && \
    make

ENV PATH="$PATH:/usr/src/dperf-custom/build"

# default command
CMD ["/usr/bin/bash"]
    