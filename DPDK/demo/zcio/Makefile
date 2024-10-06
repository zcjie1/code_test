.PHONY: custom_dpdk zcio_host zcio_docker zcio_docker_image host_app docker_app

SELF_PATH = /home/zcj/code_test/DPDK/demo/zcio/zcio_docker

host_app:
	@sudo rm -rf /tmp/zcio_server0
	@sudo rm -rf /tmp/memctl.sock
	@sudo rm -f /dev/hugepages/*
	@sudo zcio_host/build/zcio_host \
		-l 0-1 -m 1024\
		--vdev=eth_zcio0,server=1,server-iface=/tmp/zcio_server0 \
		--file-prefix=host \

docker_app:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/zcio_server0:/var/run/zcio_server0 \
		-v /tmp/memctl.sock:/var/run/memctl.sock \
		-v /dev/hugepages:/dev/hugepages \
		zcio_docker zcio_docker \
			-l 2-3 -m 1024\
			--no-pci \
			--vdev=eth_zcio1,server=0,server-iface=/var/run/zcio_server0,memctl-iface=/var/run/memctl.sock \
			--file-prefix=container \

custom_dpdk:
	cd /home/zcj && \
	tar -cvf $(SELF_PATH)/custom_dpdk.tar --exclude=dpdk-custom/build dpdk-custom

zcio_host:
	cd zcio_host && \
	make clean && \
	make static

zcio_docker_test:
	cd zcio_docker && \
	make clean && \
	make static

zcio_docker_image:
	cd zcio_docker && \
	docker build -t zcio_docker . && \
	docker image prune

dpdk_install:
	cd /home/zcj/dpdk-custom && \
	sudo rm -rf build && \
	meson setup build && \
	cd build && \
	ninja -j32 && \
	sudo meson install && \
	sudo ldconfig