FROM dpdk-base:latest

# copy source code
COPY ./* /usr/src/zcio_docker/

# compile the source file
WORKDIR /usr/src/zcio_docker
RUN make static

ENV PATH="$PATH:/usr/src/zcio_docker/build"

# default command
CMD ["/usr/bin/bash"]