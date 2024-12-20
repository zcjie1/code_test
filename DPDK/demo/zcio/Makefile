.PHONY: clean dpdk zcio scapy dpdk_install dpdk_env_image dpdk_base_image zcio_host ctl_host

DPDK_ENV_PATH = /home/zcj/code_test/DPDK/demo/dpdk_env/
DPDK_BASE_PATH = /home/zcj/code_test/DPDK/demo/dpdk_base/
ZCIO_DPERF_PATH = /home/zcj/code_test/DPDK/demo/zcio/zcio_dperf/

all: dpdk zcio
dpdk: dpdk_install dpdk_env_image dpdk_base_image
zcio: ctl_host zcio_dperf_image

scapy:
	@sudo python3 ./scapy/pktgen.py

zcio_host: clean
	@sudo ctl_host/build/ctl_host \
		-l 32-39 -m 6144\
		--proc-type=primary \
		--no-pci \
		--vdev=eth_zcio0,server=1,info-name=zcio0,queues=1 \
		--vdev=eth_zcio1,server=1,info-name=zcio1,queues=1 \

vhost_host: clean
	@sudo ctl_host/build/ctl_host \
		-l 32-39 -m 6144\
		--no-pci \
		--vdev=eth_vhost0,iface=/tmp/vhost0 \
		--vdev=eth_vhost1,iface=/tmp/vhost1 \

zcio_dperf_server:
	@docker run \
		-it --rm  \
		--privileged \
		-v /dev/hugepages:/dev/hugepages \
		-v /var/run/dpdk:/var/run/dpdk \
		-v ./zcio_dperf/server.conf:/tmp/dperf_server.conf \
		zcio_dperf dperf \
			-c /tmp/dperf_server.conf

zcio_dperf_client:
	@docker run \
		-it --rm  \
		--privileged \
		-v /dev/hugepages:/dev/hugepages \
		-v /var/run/dpdk:/var/run/dpdk \
		-v ./zcio_dperf/client.conf:/tmp/dperf_client.conf \
		zcio_dperf dperf \
			-c /tmp/dperf_client.conf

virtio_dperf_server:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/vhost0:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./virtio_dperf/server.conf:/tmp/server.conf \
		zcio_dperf dperf \
			-c /tmp/server.conf

virtio_dperf_client:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/vhost1:/var/run/usvhost0 \
		-v /dev/hugepages:/dev/hugepages \
		-v ./virtio_dperf/client.conf:/tmp/client.conf \
		zcio_dperf dperf \
			-c /tmp/client.conf

zcio_ipfilter_docker:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/zcio_server0:/var/run/zcio_server0 \
		-v /tmp/memctl.sock:/var/run/memctl.sock \
		-v /dev/hugepages:/dev/hugepages \
		zcio_ipfilter zcio_ipfilter \
			-l 12-13 -m 1024\
			--no-pci \
			--vdev=eth_zcio0,server=0,server-iface=/var/run/zcio_server0,memctl-iface=/var/run/memctl.sock \
			--file-prefix=zcio_ipfilter \

zcio_log_docker:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/zcio_server1:/var/run/zcio_server0 \
		-v /tmp/memctl.sock:/var/run/memctl.sock \
		-v /dev/hugepages:/dev/hugepages \
		-v ./zcio_log/zcio.log:/var/log/zcio.log \
		zcio_log zcio_log \
			-l 14-15 -m 1024\
			--no-pci \
			--vdev=eth_zcio0,server=0,server-iface=/var/run/zcio_server0,memctl-iface=/var/run/memctl.sock \
			--file-prefix=zcio_log \

zcio_reply_docker:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/zcio_server2:/var/run/zcio_server0 \
		-v /tmp/memctl.sock:/var/run/memctl.sock \
		-v /dev/hugepages:/dev/hugepages \
		zcio_reply zcio_reply \
			-l 16-17 -m 1024\
			--no-pci \
			--vdev=eth_zcio0,server=0,server-iface=/var/run/zcio_server0,memctl-iface=/var/run/memctl.sock \
			--file-prefix=zcio_reply \

dpdk_install:
	cd /home/zcj/dpdk-custom && \
	sudo rm -rf build && \
	meson setup build && \
	cd build && \
	ninja -j64 && \
	sudo meson install && \
	sudo ldconfig

dpdk_env_image:
	cd $(DPDK_ENV_PATH) && \
	docker build -t dpdk-env . && \
	docker image prune -f

dpdk_base_image:
	cd /home/zcj && \
	tar -cvf $(DPDK_BASE_PATH)/custom_dpdk.tar --exclude=dpdk-custom/build dpdk-custom
	cd $(DPDK_BASE_PATH) && \
	docker build -t dpdk-base . && \
	docker image prune -f

ctl_host:
	cd ctl_host && \
	make clean && \
	make static

zcio_dperf_image:
	cd /home/zcj && \
	tar -cvf $(ZCIO_DPERF_PATH)/dperf.tar \
		--exclude=dperf-custom/build \
		--exclude=dperf-custom/.vscode \
		--exclude=dperf-custom/.git \
		--exclude=dperf-custom/.github \
		dperf-custom && \
	cd $(ZCIO_DPERF_PATH) && \
	docker build -t zcio_dperf . && \
	docker image prune -f

clean:
	@sudo rm -rf /tmp/zcio_server*
	@sudo rm -rf /tmp/memctl.sock
	@sudo rm -f /dev/hugepages/*
	@sudo rm -f /tmp/vhost*
	@sudo rm -rf /var/run/dpdk/rte

