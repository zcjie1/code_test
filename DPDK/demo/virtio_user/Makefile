
.PHONY: host_app docker_app

host_app:
	sudo vhost/build/vhost \
		-l 0-2 \
		--vdev 'eth_vhost0,iface=/tmp/sock0' \
		--file-prefix=host \

docker_app:
	docker run \
		-it --rm \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		dpdk-virtio virtio-static \
			-l 6-7 -m 1024  \
			--no-pci \
			--vdev=virtio_user0,path=/var/run/usvhost0 \
			--file-prefix=container \
			