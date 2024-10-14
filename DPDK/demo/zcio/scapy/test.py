from scapy.layers.inet import IP, ICMP, UDP, TCP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sr1, srp, sr, sendp, sniff, send
from scapy.config import conf

import threading

# 定义目标IP地址
dst_ip = "192.168.10.2"  # 这里需要替换为目标机器的实际IP地址

# 创建IP层数据包
ip_packet = IP(src="192.168.10.1", dst=dst_ip)

# 添加一些负载数据（可选）
payload = "Hello, World!"

# 组合数据包
packet = ip_packet / payload

# 发送数据包的函数
def send_packet():
    send(packet, iface="ens33", loop=1, inter=1)  # 每秒发送一次

# 接收数据包的函数
def receive_packet():
    def packet_callback(packet):
        if packet.haslayer(IP) and packet[IP].src == "192.168.10.1":
            print("Received packet from:", packet[IP].src)
            print("Destination:", packet[IP].dst)
            print("Payload:", packet.load)

    sniff(iface="ens34", prn=packet_callback, filter="ip", store=0)

# 创建发送线程
send_thread = threading.Thread(target=send_packet)

# 创建接收线程
receive_thread = threading.Thread(target=receive_packet)

# 启动发送线程
send_thread.start()

# 启动接收线程
receive_thread.start()

# 等待发送线程和接收线程完成
send_thread.join()
receive_thread.join()