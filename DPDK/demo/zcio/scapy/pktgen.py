from scapy.layers.inet import IP, ICMP, UDP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sr1, srp, sr, sendp, sniff, send
from scapy.config import conf

import os
import time
import psutil
import threading
import random
import string

src_port_range = (1025, 65535)
src_port = 6666
dst_port = 8888
ip_list = [
    '192.168.3.132', '192.168.3.45', '192.168.3.200', '192.168.3.87', '192.168.3.234',
    '192.168.3.112', '192.168.3.67', '192.168.3.150', '192.168.3.99', '192.168.3.213',
    '192.168.3.104', '192.168.3.56', '192.168.3.188', '192.168.3.121', '192.168.3.245',
    '192.168.3.78', '192.168.3.162', '192.168.3.147', '192.168.3.33', '192.168.3.108',
    '192.168.3.219', '192.168.3.142', '192.168.3.158', '192.168.3.61', '192.168.3.190',
    '192.168.3.138', '192.168.3.231', '192.168.3.125', '192.168.3.52', '192.168.3.176',
    '192.168.3.249', '192.168.3.102', '192.168.3.74', '192.168.3.222', '192.168.3.183',
    '192.168.3.144', '192.168.3.115', '192.168.3.69', '192.168.3.206', '192.168.3.136',
    '192.168.3.211', '192.168.3.168', '192.168.3.48', '192.168.3.240', '192.168.3.93',
    '10.10.5.132', '10.10.5.45', '10.10.5.200', '10.10.5.87', '10.10.5.234',
    '10.10.5.112', '10.10.5.67', '10.10.5.150', '10.10.5.99', '10.10.5.213',
    '10.10.5.104', '10.10.5.56', '10.10.5.188', '10.10.5.121', '10.10.5.245',
    '10.10.5.78', '10.10.5.162', '10.10.5.147', '10.10.5.33', '10.10.5.108',
    '10.10.5.219', '10.10.5.142', '10.10.5.158', '10.10.5.61', '10.10.5.190',
    '10.10.5.138', '10.10.5.231', '10.10.5.125', '10.10.5.52', '10.10.5.176',
    '10.10.5.249', '10.10.5.102', '10.10.5.74', '10.10.5.222', '10.10.5.183',
    '10.10.5.144', '10.10.5.115', '10.10.5.69', '10.10.5.206', '10.10.5.136',
    '10.10.5.211', '10.10.5.168', '10.10.5.48', '10.10.5.240', '10.10.5.93'
]
dst_ip = "10.10.4.119"

def generate_random_string(length):
    # 选择字符集，例如ASCII字母和数字
    characters = string.ascii_letters + string.digits
    
    # 生成随机字符串
    random_string = ''.join(random.choices(characters, k=length))
    
    # 在字符串末尾添加'\0'字符
    random_string += '\0'
    
    return random_string

# port_range = (xxx, xxx)
def generate_random_port(port_range):
    return random.randint(*port_range)


def generate_udp_packet():
    custom_str = generate_random_string(1470)

    eth_layer = Ether()

    ip_layer = IP()
    source_ip = random.choice(ip_list)
    ip_layer.__setattr__('src', source_ip)
    ip_layer.__setattr__('dst', dst_ip)
    
    udp_layer = UDP()
    # random_port = generate_random_port(src_port_range)
    udp_layer.__setattr__('sport', src_port)
    udp_layer.__setattr__('dport', dst_port)

    packet = eth_layer / ip_layer / udp_layer / custom_str
    return packet

# 发送数据包的函数
def send_packet(cpu_core):
    os.sched_setaffinity(0, [cpu_core])
    while(True):
        packet = generate_udp_packet()
        sendp(packet, iface='ens33', count=1, loop=0, inter=0)
        time.sleep(1)

def receive_packet(cpu_core):
    os.sched_setaffinity(0, [cpu_core])
    def packet_callback(packet):
        if packet.haslayer(IP) and packet[IP].src == dst_ip:
        # if packet.haslayer(IP) and packet[IP].dst == dst_ip:
            print(f"[{packet[IP].src}|{packet[Ether].src}] --> [{packet[IP].dst}|{packet[Ether].dst}] : {packet.load}")
    sniff(iface="ens33", prn=packet_callback, filter="ip", store=False)

if __name__ == "__main__":
    # 创建发送线程
    send_threads = []
    for i in range(1):
        send_thread = threading.Thread(target=send_packet, args=(32 + i,))
        send_threads.append(send_thread)
        send_thread.start()

    # 创建接收线程
    receive_thread = threading.Thread(target=receive_packet, args=(35,))
    receive_thread.start()

    # 等待发送线程和接收线程完成
    for send_thread in send_threads:
        send_thread.join()

    receive_thread.join()
