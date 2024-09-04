# DPDK docker image
# docker build  -t dpdk-virtio .

FROM dpdk-base:latest

COPY ./*.c /usr/src/virtio/
COPY ./*.h /usr/src/virtio/
COPY ./Makefile /usr/src/virtio/

WORKDIR /usr/src/virtio
RUN make static && \
    cp ./build/virtio-static /usr/local/bin/

ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    