from scapy.layers.inet import IP, ICMP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sr1, srp, sr, sendp, sniff
from scapy.config import conf

import os
import time
import psutil
import threading
import random
import string

def generate_random_string(length):
    # 选择字符集，例如ASCII字母和数字
    characters = string.ascii_letters + string.digits
    
    # 生成随机字符串
    random_string = ''.join(random.choices(characters, k=length))
    
    # 在字符串末尾添加'\0'字符
    random_string += '\0'
    
    return random_string

total_send_packets = 0
total_send_bytes = 0
send_rate = 0

custom_str = generate_random_string(1479)
packet = Ether(src='11:11:11:22:22:22', dst='00:0c:29:6f:9e:fe') \
                /IP(src='192.168.187.132', dst='250.250.250.250') \
                /custom_str
packet_length = len(packet)

def set_cpu_affinity(core):
    pid = os.getpid()
    process = psutil.Process(pid)
    process.cpu_affinity(core)

def pktgen():
    global total_send_packets
    global total_send_bytes
    global packet
    global packet_length

    # 发送数据包
    send_count, _ = sendp(packet, iface ='eth1', count = 8192, verbose = 0, return_packets = True)

    total_send_packets  += send_count
    # total_send_bytes += send_count * packet_length


if __name__ == "__main__":
    set_cpu_affinity([3, 4, 5])
    print(f"Pakcet Length: {packet_length}")
    n = 0
    start_time = time.perf_counter()

    while n < 5:
        pktgen()
        n += 1
    
    end_time = time.perf_counter()
    duration_seconds = end_time - start_time

    total_send_bytes = total_send_packets * packet_length
    send_rate = total_send_bytes / 1000000 / duration_seconds
    send_rate = send_rate * 8

    print(f"Duration: {duration_seconds:.2f}s")
    print(f"Total send packets: {total_send_packets}")
    print(f"Total send bytes: {total_send_bytes}")
    print(f"Send Rate: {send_rate:.2f}Mb/s")