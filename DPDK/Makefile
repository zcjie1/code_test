# /usr/bin/bash

.PHONY: vfio_env

# 保留2M大页
hugepage2m:
	@sudo bash -c "echo 256 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages"

# 显示网卡状态
status:
	@sudo ./dpdk/usertools/dpdk-devbind.py --status

# 加载vfio-pci相关模块
vfio_env:
	@sudo modprobe vfio enable_unsafe_noiommu_mode=1
	@sudo modprobe vfio-pci
	
# eth0网卡绑定vfio-pci
vfio_bind_eth0:
	@sudo ip link set eth0 down
	@sudo ./dpdk/usertools/dpdk-devbind.py -u 0000:03:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vfio-pci 03:00.0

# eth1网卡绑定vfio-pci
vfio_bind_eth1:
	@sudo ip link set eth1 down
	@sudo ./dpdk/usertools/dpdk-devbind.py -u 0000:0b:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vfio-pci 0b:00.0

# eth2网卡绑定vfio-pci
vfio_bind_eth2:
	@sudo ip link set eth2 down
	@sudo ./dpdk/usertools/dpdk-devbind.py -u 0000:13:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vfio-pci 13:00.0

# eth3网卡绑定vfio-pci
vfio_bind_eth3:
	@sudo ip link set eth3 down
	@sudo ./dpdk/usertools/dpdk-devbind.py -u 0000:1b:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vfio-pci 1b:00.0

# eth0网卡解除vfio绑定
unbind_eth0:
	@sudo ./dpdk/usertools/dpdk-devbind.py --unbind 0000:03:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vmxnet3 03:00.0

# eth1网卡解除vfio绑定
unbind_eth1:
	@sudo ./dpdk/usertools/dpdk-devbind.py --unbind 0000:0b:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vmxnet3 0b:00.0

# eth2网卡解除vfio绑定
unbind_eth2:
	@sudo ./dpdk/usertools/dpdk-devbind.py --unbind 0000:13:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vmxnet3 13:00.0

# eth3网卡解除vfio绑定
unbind_eth3:
	@sudo ./dpdk/usertools/dpdk-devbind.py --unbind 0000:1b:00.0
	@sudo ./dpdk/usertools/dpdk-devbind.py --bind=vmxnet3 1b:00.0

clean:
	@sudo modprobe -r vfio-pci
	@sudo rmmod vfio_iommu_type1
	@sudo modprobe -r vfio