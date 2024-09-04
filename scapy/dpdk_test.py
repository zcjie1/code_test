# 1. scapy发送IP数据包（包含字符串消息）
# 2. DPDK
#     1. 工作核心1接收IP数据包
#     2. 工作核心2打印数据包的元数据和字符串消息并储存统计信息
#     3. 工作核心3广播该数据包
#     4. 主核心读取统计信息并定时打印

from scapy.layers.inet import IP, ICMP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sr1, srp, sr, sendp, sniff
from scapy.config import conf
import time
import psutil
import os

# def packet_callback(packet):
#     if Ether in packet:
#         if packet['Ether'].src == '00:0c:29:6f:9e:fe':
#             packet.show()

def set_cpu_affinity(core):
    pid = os.getpid()
    process = psutil.Process(pid)
    process.cpu_affinity(core)

def pktgen():
    # 构建IP层和字符串消息
    packet = Ether(src='11:11:11:22:22:22', dst='00:0c:29:6f:9e:fe') \
                /IP(src='192.168.187.132', dst='250.250.250.250') \
                /"Hello, From Scapy to DPDK!\0"

    # packet.show()
    
    # 发送数据包
    sendp(packet, iface ='eth1', count = 4096)

if __name__ == "__main__":
    set_cpu_affinity([3, 4])
    # interface = "eth4"
    # sniff(iface=interface, prn=packet_callback, store=0)

    while True:
        pktgen()
        time.sleep(0.1)

    