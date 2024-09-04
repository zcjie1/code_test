
.PHONY: host_app docker_app vhost virtio

host_app:
	sudo vhost/build/vhost \
		-l 0-2 \
		--vdev 'eth_vhost0,iface=/tmp/sock0' \
		--file-prefix=host \
		--log-level pmd.net.vhost:debug \
		--log-level lib.vhost.config:debug \
		--log-level lib.vhost.data:debug \

docker_app:
	docker run \
		-it --rm \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./log/virtio_packet.log:/var/log/virtio_packet.log \
		-v ./log/virtio_statistic.log:/var/log/virtio_statistic.log \
		dpdk-virtio virtio-static \
			-l 6-7 -m 1024  \
			--no-pci \
			--vdev=virtio_user0,path=/var/run/usvhost0 \
			--file-prefix=container \
			--log-level pmd.net.virtio.init:debug \
			--log-level pmd.net.virtio.driver:debug \

vhost:
	@cd vhost && \
	make clean && \
	make static

virtio:
	@docker image rm dpdk-virtio:latest && \
	cd virtio && \
	docker build -t dpdk-virtio .
			