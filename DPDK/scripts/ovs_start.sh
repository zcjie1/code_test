#!/bin/sh

export OVS_DIR=/home/zcj/code_test/DPDK/ovs/

# 创建OVS数据库配置文件
cd $OVS_DIR
mkdir -p /usr/local/etc/openvswitch
# ovsdb-tool create /usr/local/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema

# 启动OVS数据库
mkdir -p /usr/local/var/run/openvswitch
mkdir -p /usr/local/var/log/openvswitch
# ovs-vsctl --no-wait init
ovsdb-server \
    --remote=punix:/usr/local/var/run/openvswitch/db.sock \
    --remote=db:Open_vSwitch,Open_vSwitch,manager_options \
    --private-key=db:Open_vSwitch,SSL,private_key \
    --certificate=db:Open_vSwitch,SSL,certificate \
    --bootstrap-ca-cert=db:Open_vSwitch,SSL,ca_cert  \
    --pidfile --detach --log-file

# 启动ovs-vswitchd
export DB_SOCK=/usr/local/var/run/openvswitch/db.sock
ovs-vsctl --no-wait set Open_vSwitch . external_ids:system-id="test_ovs"
ovs-vsctl --no-wait set Open_vSwitch . external_ids:rundir="/usr/local/var/run/openvswitch"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-lcore-mask=3
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=12
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-hugepage-dir="/dev/hugepages"
ovs-vsctl --no-wait set Open_vSwitch . other_config:vhost-sock-dir="vhost_sock"
ovs-ctl --no-ovsdb-server --db-sock="$DB_SOCK" start

# 验证结果
ovs-ctl version
ovs-vswitchd --version
sudo ovs-vsctl get Open_vSwitch . dpdk_initialized
sudo ovs-vsctl get Open_vSwitch . dpdk_version
sudo ovs-vsctl get Open_vSwitch . other_config
sudo ovs-vsctl get Open_vSwitch . external_ids
sudo ovs-vsctl get Interface vhost0 status

# 退出OVS
sudo ovs-ctl stop


# OVS端口、桥相关操作, 参考ovs-vswitchd.conf.db手册
sudo ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
sudo ovs-vsctl del-br [bridge_name]
sudo ovs-vsctl list-br

sudo ovs-vsctl list-ports br0
sudo ovs-vsctl add-port br0 [port_name]
sudo ovs-vsctl del-port br0 [port_name]

sudo ovs-vsctl add-port br0 port0 -- set Interface port0 type=dpdk \
    options:dpdk-devargs=0000:13:00.0 ofport_request=1

sudo ovs-vsctl add-port br0 vhost1 \
    -- set Interface vhost1 type=dpdkvhostuser \
        options:ofport_request=1 \
        status:mode=server


# 流表相关操作, 参考ovs-ofctl, ovs-fields，ovs-actions手册
sudo ovs-ofctl show br0
sudo ovs-ofctl dump-flows br0
sudo ovs-ofctl add-flow br0 " \
    table=0, priority=1 \
    eth_dst=ff:ff:ff:ff:ff:ff, eth_type=0x0800, ip_dst=192.0.2.254 \
    actions=output:in_port"

sudo ovs-ofctl add-flow br0 " \
    table=0, priority=1, in_port=2 \
    actions=output:in_port"

sudo ovs-ofctl add-flow br0 " \
    table=0, priority=1, in_port=3 \
    actions=output:in_port"

sudo ovs-ofctl add-flow br0 " \
    table=0, priority=1, in_port=3 \
    actions=output:4"

sudo ovs-ofctl add-flow br0 " \
    table=0, priority=1, in_port=4 \
    actions=output:3"
