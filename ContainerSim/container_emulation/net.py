import sys
import networkx as nx
from six import string_types
from node import Node, Host, Switch,OVSSwitch,server,nginx,quagga,router,controller,ryu
from wlsnode import Station
from link import Link, Intf
from wirelesslink import AdhocIntf, AdhocwlsLink
from module import Mac80211Hwsim
from utils import ipNum, ipParse, ipStr, ipAdd, netParse
import subprocess
import json
import yaml
import os

Python3 = sys.version_info[0] == 3
BaseString = str if Python3 else getattr(str, '__base__')


class Net(object):
    def __init__(self, host=Host, station=Station, link=Link, switch=OVSSwitch, server=nginx,router=quagga,controller=ryu,intf=Intf, ipBase='10.0.0.0/8', mode='adhoc', essid='test'):
        self.host = host
        self.station = station
        self.switch = switch
        self.server = server
        self.router=router
        self.controller = controller
        self.link = link
        self.intf = intf
        self.mode = mode
        self.essid = essid
        self.nodes = []
        self.stations = []
        self.switchs = []
        self.servers = []
        self.routers = []
        self.links = []
        self.wlinks = []
        self.controllers = []
        self.container_info_list = []
        self.nameToNode = {}
        self.topo = nx.Graph()
        self.ipBase = ipBase
        self.ipBaseNum, self.prefixLen = netParse(self.ipBase)
        hostIP = (0xffffffff >> self.prefixLen) & self.ipBaseNum
        # Start for address allocation
        self.nextIP = hostIP if hostIP > 0 else 1
        print("网络已创建")

    def add_node(self, cls=None, node_type=None, node_id=None, **params):
        #  or 'ubuntu' or 'ubuntu-gedit' or 'ubuntu-vlc'
            # print(1)
        if node_type == 'station':
            self.add_station(cls=cls, node_type=node_type, node_id=node_id, **params)
        elif node_type == 'ovsswitch':
            self.add_switch(cls=cls, switch_type=node_type, switch_id=node_id, **params)
            # print(2)
        elif node_type == 'nginx':
            self.add_server(cls=nginx,node_type=node_type,node_id=node_id,**params)
            # print(3)
        elif node_type == 'quagga':
            self.add_router(cls=quagga,node_type=node_type,node_id=node_id,**params)
            # print(4)
        elif node_type == 'ryu':
            self.add_controller(cls=ryu,node_type=node_type,node_id=node_id,**params)
            # print(5)
        else:
            # print(6)
            self.add_host(cls=cls, node_type=node_type, node_id=node_id, **params)


    def prometheus_config(self):
        # print(self.container_info_list)
        apiData={
            "global":{
                "evaluation_interval":'2s',
                "scrape_interval": '2s',     
            },

            "scrape_configs" :[], 
            }
        for item in self.container_info_list:
            if item['Name']=='prometheus':
                config={
                    "job_name":item['Name'],
                    "static_configs":[{
                        "targets":["localhost:9090"] ,
                        "labels":{
                            "instance": item['Name']
                        }
                    }]
                }
            else :
                config={
                    "job_name":item['Name'],
                    "static_configs":[{
                        "targets":["{}:9100".format(item["IP Address"])] ,
                        "labels":{
                            "instance": item['Name']
                        }
                    }]
                }
            apiData["scrape_configs"].append(config)
        with open('/home/gongtao/Desktop/yamlData.yml','w',encoding='utf-8') as f:
            yaml.dump_all(documents=[apiData],stream=f,allow_unicode=True)
        self.container_info_list = []
        
    def add_host(self, cls=None, node_type=None, node_id=None, **params):
        defaults = {'ip': ipAdd(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen}
        self.nextIP += 1
        defaults.update(params)
        if not cls:
            cls = self.host
        name = str(node_type)+str(node_id)
        node = cls(name=name, nodetype=node_type, **defaults)
        self.nodes.append(node)
        self.topo.add_node(node.name)
        self.nameToNode[name] = node
        return node

    def run_docker_inspect_bridge(self):
    # 使用subprocess.run()执行命令并获取输出
        result = subprocess.run(['docker', 'network', 'inspect', 'bridge'],stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # 检查命令是否成功执行
        if result.returncode == 0:
            # 输出或处理标准输出内容
            output = result.stdout
            #print("Network Inspection Output:\n", output)
            # 这里可以进一步解析JSON输出，例如：
            network_info = json.loads(output)
            
            containers = network_info[0]['Containers']
            # print(containers)
            for node_group in [self.nodes,self.switchs,self.servers,self.routers,self.controllers,self.stations]:
                for node in node_group:
                    for container_id, container_data in containers.items():
                        #print(f"Name:{container_data['Name']}, IP Address: {container_data['IPv4Address']}")
                        ip_address = container_data['IPv4Address'].split('/')[0]
                        # if ip_address.split('.')[3] > '5':
                        if container_data['Name'] == node.name:
                            container_info = {
                                "Name": container_data['Name'],
                                "IP Address": ip_address
                            }
                            self.container_info_list.append(container_info)
        else:
            # 如果命令执行失败，打印错误信息
            print("Error running docker command:", result.stderr)
        #print("Container Information List:\n", container_info_list)
            
    def del_host(self, node):
        node = node if not isinstance(node, BaseString) else self.get(node)
        links = [link for link in self.links if node in link.nodes]
        for link in links:
            self.del_link(link)
        node.remove_node()
        self.nodes.remove(node)
        self.topo.remove_node(node.name)

    def add_station(self, cls=None, node_type=None, node_id=None, **params):
        defaults = {'ip': ipAdd(self.nextIP,
                                ipBaseNum=self.ipBaseNum,
                                prefixLen=self.prefixLen) +
                          '/%s' % self.prefixLen,
                    'mode': self.mode,
                    'essid': self.essid
                    }
        self.nextIP += 1
        defaults.update(params)
        if not cls:
            cls = self.station
        name = str(node_type)+str(node_id)
        station = cls(name=name, nodetype=node_type, **defaults)
        self.addWlans(station)
        self.stations.append(station)
        self.topo.add_node(station.name)
        self.nameToNode[name] = station
        return station

    def addWlans(self, node):
        node.params['wlan'] = []
        wlans = node.params.get('wlans', 1)
        for wlan in range(wlans):
            wlan_id = wlan
            # if isinstance(node, AP):
                # wlan_id += 1
            # node.params['wlan'].append(node.name + '-wlan' + str(wlan_id))
            node.params['wlan'].append('wlan' + str(wlan_id))
        node.params.pop("wlans", None)

        # creates hwsim interfaces on the fly
        # if Mac80211Hwsim.hwsim_ids:
        Mac80211Hwsim(node=node, on_the_fly=True)
        mode = node.params.get('mode', self.mode)
        if mode == 'adhoc':
            self.set_adhoc_node(node)

    def del_station(self, station):
        station = station if not isinstance(station, BaseString) else self.get(station)
        links = [link for link in self.links if station in link.nodes]
        for link in links:
            self.del_link(link)
        station.remove_node()
        self.stations.remove(station)
        self.topo.remove_node(station.name)

    def config_Node(self, node, intf_name, mac=None, ip=None, ipv6=None,**kwargs):
        result=''
        intf = node.intf(intf_name)
        if not intf:
            print("节点 {}不存在端口 {}".format(node.name, intf_name))
            return
        if mac:
            result+=intf.setMAC(mac)
        if ip:
            result+=intf.setIP(ip)
        if ipv6:
            result+=intf.setIPv6(ipv6)
        return result

    def set_adhoc_node(self, node):
        for wlan in range(len(node.params['wlan'])):
            intf = node.params['wlan'][wlan]
            wintf = AdhocIntf(name=intf, node=node, port=wlan, essid=node.essid, mac=node.mac, ip=node.ip)
            for wlink in self.wlinks:
                if wlink.essid == wintf.essid:
                    wlink.add_wintf(wintf)
                    pass
                else:
                    link = AdhocwlsLink(essid=node.essid)
                    self.wlinks.append(link)
        # for intf in node.wintfs.values():
            # intf.setMAC(intf.mac)
            # intf.setIP(intf.ip, intf.prefixLen)

    def open_node_xterm(self, node):
        node = node if not isinstance(node, BaseString) else self.get(node)
        node.open_xterm()

    def add_switch(self, cls=None, switch_type=None, switch_id=None, **params):
        if not cls:
            cls = self.switch
        name = str(switch_type)+str(switch_id)
        switch = cls(name=name, switch_type=switch_type, **params)
        self.switchs.append(switch)
        self.topo.add_node(switch.name)
        self.nameToNode[name] = switch
        return switch

    def del_switch(self, switch):
        switch = switch if not isinstance(switch, BaseString) else self.get(switch)
        links = [link for link in self.links if switch in link.nodes]
        for link in links:
            self.del_link(link)
        switch.remove_node()
        self.switchs.remove(switch)
        self.topo.remove_node(switch.name)

    def add_server(self,cls=None,node_type=None, node_id=None, **params):
        if not cls:
            cls=self.server
        name = str(node_type)+str(node_id)
        server = cls(name=name,nodetype=node_type,**params)
        self.servers.append(server)
        self.topo.add_node(server.name)
        self.nameToNode[name] = server
        return server

    def del_server(self, server):
        server = server if not isinstance(server, BaseString) else self.get(server)
        links = [link for link in self.links if server in link.nodes]
        for link in links:
            self.del_link(link)
        server.remove_node()
        self.servers.remove(server)
        self.topo.remove_node(server.name)
    
    def add_router(self,cls=None,node_type=None, node_id=None, **params):
        if not cls:
            cls=self.router
        name = str(node_type)+str(node_id)
        router = cls(name=name,nodetype=node_type,**params)
        self.routers.append(router)
        self.topo.add_node(router.name)
        self.nameToNode[name] = router
        return router

    def del_router(self, router):
        router = router if not isinstance(router, BaseString) else self.get(router)
        links = [link for link in self.links if router in link.nodes]
        for link in links:
            self.del_link(link)
        router.remove_node()
        self.routers.remove(router)
        self.topo.remove_node(router.name)

    def add_controller(self,cls=None,node_type=None, node_id=None, **params):
        if not cls:
            cls=self.controller
        name = str(node_type)+str(node_id)
        controller = cls(name=name,nodetype=node_type,**params)
        self.controllers.append(controller)
        self.topo.add_node(controller.name)
        self.nameToNode[name] = controller
        return controller

    def del_controller(self, controller):
        controller = controller if not isinstance(controller, BaseString) else self.get(controller)
        links = [link for link in self.links if controller in link.nodes]
        for link in links:
            self.del_link(link)
        controller.remove_node()
        self.controllers.remove(controller)
        self.topo.remove_node(controller.name)

    def del_node(self, node):
        if isinstance(node, Host):
            self.del_host(node)
        elif isinstance(node, Switch):
            self.del_switch(node)
        elif isinstance(node,server):
            self.del_server(node)
        elif isinstance(node,router):
            self.del_router(node)
        elif isinstance(node,controller):
            self.del_controller(node)

    def getNodeByName(self, *args):
        "Return node(s) with given name(s)"
        if len(args) == 1:
            return self.nameToNode[args[0]]
        return [self.nameToNode[n] for n in args]

    def get(self, *args):
        "Convenience alias for getNodeByName"
        return self.getNodeByName(*args)

    def add_link(self, node1, node2, cls=None, **options):
        node1 = node1 if not isinstance(node1, BaseString) else self.get(node1)
        node2 = node2 if not isinstance(node2, BaseString) else self.get(node2)
        cls = self.link if cls is None else cls
        link = cls(node1, node2, **options)
        self.links.append(link)
        self.topo.add_edge(node1.name, node2.name)
        return link

    def del_link(self, link):
        self.topo.remove_edge(link.nodes[0].name, link.nodes[1].name)
        link.del_link()
        self.links.remove(link)

    def del_topo(self):
        nodes = []
        stations = []
        switchs = []
        servers = []
        routers = []
        controllers = []
        for node in self.nodes:
            nodes.append(node)
        for node in nodes:
            self.del_node(node)
        if self.stations:
            for station in self.stations:
                stations.append(station)
            subprocess.run('modprobe -r mac80211_hwsim',shell=True,stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        for station in stations:
            self.del_station(station)
        for switch in self.switchs:
            switchs.append(switch)
        for switch in switchs:
            self.del_switch(switch)
        for server in self.servers:
            servers.append(server)
        for server in servers:
            self.del_server(server)
        for router in self.routers:
            routers.append(router)
        for router in routers:
            self.del_router(router)
        for controller in self.controllers:
            controllers.append(controller)
        for controller in controllers:
            self.del_controller(controller)
        print("已删除全部节点")

        return None

