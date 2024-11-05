#!/bin/sh

ovs-ctl --no-ovs-vswitchd start
mkdir -p /var/run/openvswitch/vhost_sock

# 启动ovs-vswitchd
export DB_SOCK=/var/run/openvswitch/db.sock
ovs-vsctl --no-wait set Open_vSwitch . external_ids:system-id="virtio_test"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-lcore-mask=3
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=12
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-hugepage-dir="/dev/hugepages"
ovs-vsctl --no-wait set Open_vSwitch . other_config:vhost-sock-dir="vhost_sock"
ovs-ctl start


