from scapy.layers.inet import IP, ICMP
from scapy.layers.l2 import Ether
from scapy.sendrecv import sr1, srp, sr
from scapy.config import conf

def packet_callback(packet):
    if IP in packet and ICMP in packet:
        print("Received packet:")
        packet.show()

def ping_host(target):
    # 构建IP层和ICMP层
    packet = IP(dst=target)/ICMP()

    packet.show()
    
    # 发送数据包并等待响应
    response = sr1(packet, timeout=2, verbose=True)

    if response:
        # 如果收到响应，则打印出来
        print("Response received:")
        print(response)
        response.show()
    else:
        print("No response received.")

if __name__ == "__main__":
    target = "192.168.187.130"
    ping_host(target)

    # 开始监听所有数据包
    # interface = "eth3"
    # sniff(iface=interface, prn=packet_callback, store=0)