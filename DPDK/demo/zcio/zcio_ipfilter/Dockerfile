FROM dpdk-base:latest

# copy source code
COPY ./* /usr/src/zcio_ipfilter/

# compile the source file
WORKDIR /usr/src/zcio_ipfilter
RUN make static

ENV PATH="$PATH:/usr/src/zcio_ipfilter/build"

# default command
CMD ["/usr/bin/bash"]