# DPDK docker image
# docker build -f ./virtio/Dockerfile -t dpdk-virtio ~/code_test/DPDK/

FROM dpdk-base:latest

COPY ./virtio.c /usr/src/virtio/
COPY ./Makefile /usr/src/virtio/

WORKDIR /usr/src/virtio
RUN make static && \
    cp ./build/virtio-static /usr/local/bin/

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    