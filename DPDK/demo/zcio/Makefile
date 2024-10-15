.PHONY: clean dpdk zcio dpdk_install dpdk_env_image dpdk_base_image zcio_host zcio_docker_image zcio_ipfilter_image zcio_log_image zcio_reply_image

DPDK_ENV_PATH = /home/zcj/code_test/DPDK/demo/dpdk_env/
DPDK_BASE_PATH = /home/zcj/code_test/DPDK/demo/dpdk_base/

all: dpdk zcio
dpdk: dpdk_install dpdk_env_image dpdk_base_image
zcio: zcio_host zcio_docker_image zcio_ipfilter_image zcio_log_image zcio_reply_image

host_app: clean
	@sudo zcio_host/build/zcio_host \
		-l 0-11 -m 4096\
		--vdev=eth_zcio0,server=1,server-iface=/tmp/zcio_server0 \
		--vdev=eth_zcio1,server=1,server-iface=/tmp/zcio_server1 \
		--vdev=eth_zcio2,server=1,server-iface=/tmp/zcio_server2 \
		--file-prefix=host \

zcio_ipfilter_docker:
	@docker run \
		-it --rm  \
		--privileged \
		-v /tmp/zcio_server0:/var/run/zcio_server0 \
		-v /tmp/memctl.sock:/var/run/memctl.sock \
		-v /dev/hugepages:/dev/hugepages \
		zcio_ipfilter zcio_ipfilter\ \
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

zcio_host:
	cd zcio_host && \
	make clean && \
	make static

zcio_docker_image:
	cd zcio_docker && \
	make clean && \
	make static && \
	docker build -t zcio_docker . && \
	docker image prune -f

zcio_ipfilter_image:
	cd zcio_ipfilter && \
	make clean && \
	make static && \
	docker build -t zcio_ipfilter . && \
	docker image prune -f

zcio_log_image:
	cd zcio_log && \
	make clean && \
	make static && \
	docker build -t zcio_log . && \
	docker image prune -f

zcio_reply_image:
	cd zcio_reply && \
	make clean && \
	make static && \
	docker build -t zcio_reply . && \
	docker image prune -f

clean:
	@sudo rm -rf /tmp/zcio_server*
	@sudo rm -rf /tmp/memctl.sock
	@sudo rm -f /dev/hugepages/*

