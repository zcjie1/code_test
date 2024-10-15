FROM dpdk-base:latest

# copy source code
COPY ./* /usr/src/zcio_reply/

# compile the source file
WORKDIR /usr/src/zcio_reply
RUN make static

ENV PATH="$PATH:/usr/src/zcio_reply/build"

# default command
CMD ["/usr/bin/bash"]