
# 配置veth网卡，只可以名为veth0,veth1
set_veth:
	sudo bash -c "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter"
	sudo bash -c "echo 0 > /proc/sys/net/ipv4/conf/veth0/rp_filter"
	sudo bash -c "echo 0 > /proc/sys/net/ipv4/conf/veth1/rp_filter"
	sudo bash -c "echo 1 > /proc/sys/net/ipv4/conf/veth0/accept_local"
	sudo bash -c "echo 1 > /proc/sys/net/ipv4/conf/veth1/accept_local"

change_src_ip:
	sudo bash -c "iptables -t nat -A POSTROUTING -o veth0 -j SNAT --to-source 1.2.3.4"