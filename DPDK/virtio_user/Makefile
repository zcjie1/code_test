
.PHONY: host_app docker_app

host_app:
	sudo ./host_app/dpdk-testpmd \
		-l 0-1 \
		--socket-mem 1024 \
		--vdev=eth_vhost0,iface=/tmp/sock0 \
		--vdev=eth_vhost1,iface=/tmp/sock1 \
		--file-prefix=host \
		--no-pci \
		-- \
		--portmask=0x3 \
		--no-numa \
		-i

docker_app:
	docker run \
		-it --rm \
		--privileged \
		-v /tmp/sock0:/var/run/usvhost0 \
		-v /tmp/sock1:/var/run/usvhost1 \
		-v /dev/hugepages:/dev/hugepages \
		-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
		dpdk-dev dpdk-testpmd \
			-l 6-7 -m 1024 --no-pci \
			--vdev=virtio_user0,path=/var/run/usvhost0 \
			--vdev=virtio_user1,path=/var/run/usvhost1 \
			--file-prefix=container \
			-- \
			--portmask=0x3 \
			--no-numa \
			-i