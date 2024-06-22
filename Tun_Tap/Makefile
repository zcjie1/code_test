# TUN/TAP虚拟设备测试代码

# 当未有程序(比如tun_count)读取TUN设备时，发往TUN设备的数据包都会被丢弃
# tcpdump无法抓取到对应数据包

# 对于TAP设备,当启动TAP设备后
# 若Ping物理网卡, 则tap_count程序会将ARP request拦截
# 若手动设置ARP后，则tap_count程序会将ICMP请求拦截

default:
	gcc -o tun_count tun_count.c
	gcc -o tap_count tap_count.c

.PHONY: create_tun

# 创建TUN设备, 并配置IP地址, 激活设备
# 第三条指令默认添加对应子网的路由规则
create_tun:
	sudo ip tuntap add dev test_tun mode tun
	sudo ip addr add 192.168.1.1/24 dev test_tun
	sudo ip link set dev test_tun up

# 激活TUN设备
activate_tun:
	sudo ip link set dev test_tun up

# 设置TUN设备路由规则
set_route:
	sudo ip route add 192.168.2.0/24 dev test_tun

# 删除TUN设备路由规则
delete_route:
	sudo ip route del 192.168.2.0/24 dev test_tun

# 删除TUN设备
delete_tun:
	sudo ip tuntap del dev test_tun mode tun

# 可通过 ip addr show 查看当前网卡设备
# 可通过 ip route show table all 或者 ip route list 查看当前所有路由规则
# 可通过 tcpdump -i test_tun 抓取通过test_tun的设备

clean:
	rm tun_count tap_count