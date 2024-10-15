FROM dpdk-base:latest

# copy source code
COPY ./* /usr/src/zcio_log/

# compile the source file
WORKDIR /usr/src/zcio_log
RUN make static

ENV PATH="$PATH:/usr/src/zcio_log/build"

# default command
CMD ["/usr/bin/bash"]