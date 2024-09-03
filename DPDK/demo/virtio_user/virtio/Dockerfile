# DPDK docker image 

FROM ubuntu:latest

# copy DPDK source code
COPY ./build/virtio-static /usr/local/bin

# install DPDK
WORKDIR /usr/local/bin
    
# dpdk-testpmd path
ENV PATH="$PATH:/usr/local/bin"

# default command
CMD ["/usr/bin/bash"]
    