.PHONY: clean dpdk zcio scapy dpdk_env_image dpdk_base_image zcio_host ctl_host

DPDK_ENV_PATH = /home/zcj/code_test/DPDK/demo/dpdk_env/
DPDK_BASE_PATH = /home/zcj/code_test/DPDK/demo/dpdk_base/
ZCIO_DPERF_PATH = /home/zcj/code_test/DPDK/demo/zcio/zcio_dperf/
CUSTOM_DPDK_PATH = dpdk

COMPILE_COMMANDS = /home/zcj/code_test/DPDK/compile_commands.json

all: dpdk zcio
dpdk: dpdk_env_image dpdk_base_image
zcio: zcio_dperf_image router

scapy:
	@sudo python3 ./scapy/pktgen.py

zcio_host: clean
	@sudo HostRouter/build/router \
		HostRouter/zcio.ini

vhost_host: clean
	@sudo HostRouter/build/router \
		HostRouter/virtio.ini

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

zcio_fstack_server:
	cd f-stack && \
	make zcio_server

zcio_fstack_client:
	cd f-stack && \
	make zcio_client

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

virtio_fstack_server:
	cd f-stack && \
	make demo_server

virtio_fstack_client:
	cd f-stack && \
	make demo_client

router:
	cd HostRouter && \
	make clean && \
	bear --output $(COMPILE_COMMANDS) --append -- make static -j64

dpdk_install:
	cd f-stack && \
	make dpdk_install

dpdk_env_image:
	cd $(DPDK_ENV_PATH) && \
	docker build -t dpdk-env . && \
	docker image prune -f

dpdk_base_image:
	cd /home/zcj/f-stack/ && \
	tar -cvf $(DPDK_BASE_PATH)/custom_dpdk.tar --exclude=$(CUSTOM_DPDK_PATH)/build $(CUSTOM_DPDK_PATH)
	cd $(DPDK_BASE_PATH) && \
	docker build -t dpdk-base . && \
	docker image prune -f

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

