from scapy.layers.inet import IP, ICMP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sendp
from scapy.config import conf
import os
import time
import psutil
import multiprocessing
import random
import string
from decimal import Decimal, getcontext

def generate_random_string(length):
    # 选择字符集，例如ASCII字母和数字
    characters = string.ascii_letters + string.digits
    
    # 生成随机字符串
    random_string = ''.join(random.choices(characters, k=length))
    
    # 在字符串末尾添加'\0'字符
    random_string += '\0'
    
    return random_string

# 初始化全局变量
total_send_packets = multiprocessing.Value('i', 0)
# total_send_bytes = multiprocessing.Value('i', 0)
total_send_bytes = 0
send_rate = Decimal('0.0')

# 00:0c:29:6f:9e:fe eth0
# 00:0c:29:6f:9e:08 eth1
# 00:50:56:c0:00:01 VMnet1
# 11:11:11:22:22:22 custom
custom_str = generate_random_string(1479)
packet = Ether(src='00:0c:29:6f:9e:08', dst='00:50:56:c0:00:01') \
         /IP(src='192.168.187.132', dst='250.250.250.250') \
         /custom_str
packet_length = len(packet)

epoch = 5

def set_cpu_affinity(core):
    pid = os.getpid()
    process = psutil.Process(pid)
    process.cpu_affinity(core)

def atomic_increment(counter, value):
    """ 使用CAS机制来模拟原子加操作 """
    with counter.get_lock():
        counter.value += value

def pktgen(core):
    global total_send_packets
    global total_send_bytes
    global packet
    global packet_length
    global epoch

    set_cpu_affinity([core])

    n = 0
    while n < epoch:
        # 发送数据包
        send_count, _ = sendp(packet, iface='eth1', count=15000, verbose=0, return_packets=True)

        # 原子地增加计数器
        atomic_increment(total_send_packets, send_count)
        # atomic_increment(total_send_bytes, send_count * packet_length)
        n += 1

if __name__ == "__main__":
    # set_cpu_affinity([3, 4, 5])
    print(f"Packet Length: {packet_length}")
    start_time = time.perf_counter()

    # 创建进程列表
    processes = []

    # 启动多个进程
    for core in [3, 4, 5, 6]:
        p = multiprocessing.Process(target=pktgen, args=(core,))
        p.start()
        processes.append(p)

    # 等待所有进程完成
    for p in processes:
        p.join()

    end_time = time.perf_counter()
    duration_seconds = Decimal(str(end_time - start_time))
    total_send_bytes = total_send_packets.value * packet_length
    send_rate = Decimal(str(total_send_bytes)) * Decimal('8') / (duration_seconds * Decimal('1000000'))

    print(f"Duration: {duration_seconds:.2f}s")
    print(f"Total Send Packets: {total_send_packets.value}")
    print(f"Total Send Bytes: {total_send_bytes}")
    print(f"Send Rate: {send_rate:.2f}Mb/s")