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

port_range = (1025, 65535)
# src_port = 6666
# dst_port = 8888

ip_white_list =[
    '192.168.3.132', '192.168.3.45', '192.168.3.200', '192.168.3.87', '192.168.3.234',
    '192.168.3.112', '192.168.3.67', '192.168.3.150', '192.168.3.99', '192.168.3.213',
    '192.168.3.104', '192.168.3.56', '192.168.3.188', '192.168.3.121', '192.168.3.245',
    '192.168.3.78', '192.168.3.162', '192.168.3.147', '192.168.3.33', '192.168.3.108',
    '192.168.3.219', '192.168.3.142', '192.168.3.158', '192.168.3.61', '192.168.3.190',
    '192.168.3.138', '192.168.3.231', '192.168.3.125', '192.168.3.52', '192.168.3.176',
    '192.168.3.249', '192.168.3.102', '192.168.3.74', '192.168.3.222', '192.168.3.183',
    '192.168.3.144', '192.168.3.115', '192.168.3.69', '192.168.3.206', '192.168.3.136',
    '192.168.3.211', '192.168.3.168', '192.168.3.48', '192.168.3.240', '192.168.3.93',
]

ip_black_list = [
    '10.10.5.132', '10.10.5.45', '10.10.5.200', '10.10.5.87', '10.10.5.234',
    '10.10.5.112', '10.10.5.67', '10.10.5.150', '10.10.5.99', '10.10.5.213',
    '10.10.5.104', '10.10.5.56', '10.10.5.188', '10.10.5.121', '10.10.5.245',
    # '10.10.5.78', '10.10.5.162', '10.10.5.147', '10.10.5.33', '10.10.5.108',
    # '10.10.5.219', '10.10.5.142', '10.10.5.158', '10.10.5.61', '10.10.5.190',
    # '10.10.5.138', '10.10.5.231', '10.10.5.125', '10.10.5.52', '10.10.5.176',
    # '10.10.5.249', '10.10.5.102', '10.10.5.74', '10.10.5.222', '10.10.5.183',
    # '10.10.5.144', '10.10.5.115', '10.10.5.69', '10.10.5.206', '10.10.5.136',
    # '10.10.5.211', '10.10.5.168', '10.10.5.48', '10.10.5.240', '10.10.5.93'
]

dst_ip = "10.10.4.119"
dst_mac = '00:50:56:bf:a9:7c'

log = open("packet.log", "w+")

# 统计数据
stats = {
    'total_packets': 0,
    'total_bytes': 0,
    'start_time': time.time(),
}

stats_lock = threading.Lock()

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

def generate_udp_packet(source_ip_list):
    custom_str = generate_random_string(1460)

    eth_layer = Ether()
    eth_layer.__setattr__('dst', dst_mac)

    ip_layer = IP()
    source_ip = random.choice(source_ip_list)
    ip_layer.__setattr__('src', source_ip)
    ip_layer.__setattr__('dst', dst_ip)
    
    udp_layer = UDP()
    src_port = generate_random_port(port_range)
    dst_port = generate_random_port(port_range)
    udp_layer.__setattr__('sport', src_port)
    udp_layer.__setattr__('dport', dst_port)

    packet = eth_layer / ip_layer / udp_layer / custom_str
    return packet

def generate_udp_packets_list(source_ip_list):
    packets_list = [generate_udp_packet(source_ip_list) for _ in range(4096)]
    return packets_list

# 发送有效数据包的函数
def send_valid_packet(cpu_core):
    os.sched_setaffinity(0, [cpu_core])
    packet = generate_udp_packets_list(ip_white_list)
    # print("Start send valid packet")
    while(True):
        sendp(packet, iface='ens33', count=64, loop=0, inter=0, verbose=0)
        # with stats_lock:
        #     stats['total_packets'] += len(packet_list)
        #     stats['total_bytes'] += sum(len(p) for p in packet_list)

# 发送无效数据包的函数
def send_invalid_packet(cpu_core):
    os.sched_setaffinity(0, [cpu_core])
    packet = generate_udp_packets_list(ip_black_list)
    # print("Start send invalid packet")
    while(True):
        sendp(packet, iface='ens33', count=1, loop=0, inter=0, verbose=0)
        # with stats_lock:
        #     stats['total_packets'] += len(packet_list)
        #     stats['total_bytes'] += sum(len(p) for p in packet_list)

def receive_packet(cpu_core):
    os.sched_setaffinity(0, [cpu_core])
    def packet_callback(packet):
        if packet.haslayer(IP) and packet[IP].src == dst_ip:
            print(f"Receive [{packet[IP].src}|{packet[Ether].src}] --> [{packet[IP].dst}|{packet[Ether].dst}] : {packet.load}", file=log)
    sniff(iface="ens34", prn=packet_callback, filter="ip", store=False)

# def output_stats():
#     print("Start output stats")
#     while True:
#         current_time = time.time()
#         elapsed_time = current_time - stats['start_time']
#         total_packets = stats['total_packets']
#         total_bytes = stats['total_bytes']
#         if elapsed_time > 0:
#             packet_rate = total_packets / elapsed_time
#             byte_rate = total_bytes / elapsed_time
#         else:
#             packet_rate = 0
#             byte_rate = 0
#         print(f"Elapsed: {elapsed_time:.2f}s, Total Packets: {total_packets}, Packet Rate: {packet_rate:.2f} pps, Total Bytes: {total_bytes}, Byte Rate: {byte_rate:.2f} B/s")
#         time.sleep(5)

if __name__ == "__main__":

    # 创建输出统计信息的线程
    # stats_thread = threading.Thread(target=output_stats)
    # stats_thread.daemon = True
    # stats_thread.start()
    
    # 创建发送线程 - 有效数据包
    send_threads = []
    for i in range(8):
        send_thread = threading.Thread(target=send_valid_packet, args=(32 + i,))
        send_threads.append(send_thread)
        send_thread.start()
    
    # print("mark1")
    # 创建发送线程 - 无效数据包
    for i in range(1):
        send_thread = threading.Thread(target=send_invalid_packet, args=(40 + i,))
        send_threads.append(send_thread)
        send_thread.start()
    
    # print("mark2")
    # 创建接收线程
    receive_thread = threading.Thread(target=receive_packet, args=(48,))
    receive_thread.start()
    
    # print("mark3")
    # 等待发送线程和接收线程完成
    for send_thread in send_threads:
        send_thread.join()

    # print("mark4")
    receive_thread.join()
    
    # stats_thread.join()

    log.close()
