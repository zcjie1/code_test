
.PHONY: hping3 pktgen host_app docker_app vhost virtio virtio_dperf_server virtio_dperf_client

host_app:
	@sudo rm -rf /tmp/sock0
	@sudo vhost/build/vhost \
		-l 0-2 -m 1024\
		--vdev 'eth_vhost0,iface=/tmp/sock0' \
		--file-prefix=host \
		--log-level lib.eal:debug \
		--log-level pmd.net.vhost:debug \
		--log-level lib.vhost.config:debug \
		--log-level lib.vhost.data:debug

# /usr/local/var/run/openvswitch/vhost_sock/vhost0
docker_app:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./log/virtio_packet.log:/var/log/virtio_packet.log \
		-v ./log/virtio_statistic.log:/var/log/virtio_statistic.log \
		dpdk-virtio virtio-static \
			-l 9-11 -m 1024\
			--no-pci \
			--vdev=virtio_user0,path=/var/run/usvhost0,queue_size=1024 \
			--file-prefix=container \
			--log-level lib.mempool:debug \
			--log-level lib.mbuf:debug \
			--log-level lib.ethdev:debug \
			--log-level lib.net:debug \
			--log-level lib.port:debug \
			--log-level pmd.net.virtio.init:debug \
			--log-level pmd.net.virtio.driver:debug \
			--log-level pmd:debug \

virtio_dperf_server:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./virtio_dperf/server.conf:/tmp/server.conf \
		virtio_dperf dperf \
			-c /tmp/server.conf

virtio_dperf_client:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./virtio_dperf/client.conf:/tmp/client.conf \
		virtio_dperf dperf \
			-c /tmp/client.conf
			

hping3:
	sudo hping3 --flood -I eth1 --rand-dest -t 1 -d 1460 --verbose 192.168.116.128

pktgen:
	sudo ./pktgen/pktgen_sample02_multiqueue.sh -i eth1 -f 4 -t 2 -m 00:0c:29:6f:9e:fe -d 192.168.116.128 -s 1500

# compile the program
all: vhost virtio		

vhost:
	@cd vhost && \
	make clean && \
	make static

virtio:
	@docker image rm dpdk-virtio:latest && \
	cd virtio && \
	docker build -t dpdk-virtio .
			