FROM dpdk-base:latest

COPY ./demo/*.conf /usr/src/dperf/demo/
COPY ./src/* /usr/src/dperf/src/
COPY ./Makefile /usr/src/dperf/

WORKDIR /usr/src/dperf
RUN make -j$(nproc) && \
    cp ./build/dperf /usr/local/bin/

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    