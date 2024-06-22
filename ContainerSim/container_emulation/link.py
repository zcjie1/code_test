import re
import socket
from pyroute2 import NDB, IPRoute

class Intf(object):
    def __init__(self, name, node=None, port=None, link=None,
                  mac=None, **params):
        self.node = node
        self.name = name
        self.link = link
        self.mac = mac
        self.ip, self.prefixLen = None, None
        self.ipv6 ,self.prefixLenv6 =None,None
        if self.name == 'lo':
            self.ip = '127.0.0.1'
            self.prefixLen = 8
        if node:
            node.addIntf(self, port=port)
        self.params = params
        self.config(**params)

    def cmd(self, *args):
        return self.node.cmd(*args)

    def config(self, **params):
        mac = params.get('mac', None)
        ip = params.get('ip', None)
        r = {}
        self.setparam(r, 'setMAC', mac=mac)
        self.setparam(r, 'setIP', ip=ip)
        # self.setparam(r, 'isUp', up=up)
        # self.setparam(r, 'ifconfig', ifconfig=ifconfig)
        return r

    def setparam(self, results, method, **param):
        name, value = list(param.items())[0]
        f = getattr(self, method, None)
        if not f or value is None:
            return None
        if isinstance(value, list):
            result = f(*value)
        elif isinstance(value, dict):
            result = f(**value)
        else:
            result = f(value)
        results[name] = result
        return result

    def setMAC(self, macstr):
        if not macstr:
            print("未设置mac地址")
            return
        self.mac = macstr
        with NDB(log='stdout') as ndb:
            ndb.sources.add(netns=self.node.name)
            with ndb.interfaces[{'target': self.node.name, 'ifname': self.name}] as interface:
                interface.set('address', macstr)
        print("节点 {}的{}端口的mac地址：{}已配置".format(self.node.name, self.name, self.mac))

    def setIP(self, ipstr, prefixLen=None):
        if not ipstr:
            print("未设置ip地址")
            return
        # print(ipstr)
        try:
            with NDB(log='stdout') as ndb:
                ndb.sources.add(netns=self.node.name)
                with ndb.interfaces[{'target': self.node.name, 'ifname': self.name}] as interface:
                    if self.ip:
                        interface.del_ip(family=socket.AF_INET)
                    if '/' in ipstr:
                        self.ip, self.prefixLen = ipstr.split('/')
                    else:
                        if prefixLen is None:
                            raise Exception('No prefix length set for IP address %s'
                                            % (ipstr,))
                        self.ip, self.prefixLen = ipstr, prefixLen
                    interface.add_ip(address=self.ip, prefixlen=self.prefixLen,family=socket.AF_INET)
            print("节点 {}的{}端口的ip地址：{}/{}已配置".format(self.node.name, self.name, self.ip, self.prefixLen))
            return "ipv4"
        except Exception as e:
            print(e,'ipv4配置错误')
    
    def setIPv6(self, ipstr, prefixLenv6=None):
        if not ipstr:
            print("未设置IPv6地址")
            return
        # try:
        with NDB(log='stdout') as ndb:
            ndb.sources.add(netns=self.node.name)
            with ndb.interfaces[{'target': self.node.name, 'ifname': self.name}] as interface:
                if self.ipv6:
                    interface.del_ip(family=socket.AF_INET6)
                if '/' in ipstr:
                    self.ipv6, self.prefixLenv6 = ipstr.split('/')
                else:
                    if prefixLenv6 is None:
                        raise Exception('No prefix length set for IPv6 address %s'
                                        % (ipstr,))
                    self.ipv6, self.prefixLenv6 = ipstr, prefixLenv6
                # interface.ipaddr.create("beef:feed::1/112").commit()
                # interface.add_ip(address=ipstr, family=socket.AF_INET6)
                interface.add_ip(address=self.ipv6, prefixlen=self.prefixLenv6,family=socket.AF_INET6)
        print("节点 {}的{}端口的IPv6地址：{}/{}已配置".format(self.node.name, self.name, self.ipv6, self.prefixLenv6))
        return "ipv6"
        # except Exception as e:
        #     print(e,'ipv6配置错误')

    def IP(self):
        return self.ip

    def MAC(self):
        return self.mac

    def rename(self, newname):
        if self.node and self.name in self.node.nameToIntf:
            self.node.nameToIntf[newname] = self.node.nameToIntf.pop(self.name)
        result = self.cmd('ip link set' + self.name + 'name' + newname)
        self.name = newname
        return result

    def delete_selflink(self):
        self.link = None

    def delete(self):
        against_intf = self.link.intf1 if self.link.intf1.name == self.name else self.link.intf2
        against_intf.delete_selflink()
        if self.link:
            with NDB(log='stdout') as ndb:
                ndb.sources.add(netns=self.node.name)
                interface = ndb.interfaces[{'target': self.node.name, 'ifname': self.name}]
                interface.remove()
        self.node.delIntf(self)
        self.delete_selflink()
        print("节点 {}的 {}端口已删除".format(self.node.name, self.name))

    def __str__(self):
        return self.name

    # _ipMatchRegex = re.compile(r'\d+\.\d+\.\d+\.\d+')
    # _macMatchRegex = re.compile(r'..:..:..:..:..:..')


class Link(object):
    def __init__(self, node1, node2,
                 intf=Intf, addr1=None, addr2=None):
        self.nodes = [node1, node2]
        self.intf_name1 = 'to' + node2.name
        self.intf_name2 = 'to' + node1.name
        self.add_link(node1, node2, addr1, addr2)
        self.intf1 = intf(name=self.intf_name1, node=node1, link=self, mac=addr1)
        self.intf2 = intf(name=self.intf_name2, node=node2, link=self, mac=addr2)

    def add_link(self, node1, node2, addr1, addr2):
        ipr = IPRoute()
        ipr.link('add', ifname='veth1', kind='veth', peer='veth2')
        if node1.type=='ovsswitch':
            node1.get_instance().exec_run(["sh","-c","ovs-vsctl add-port br {}".format(self.intf_name1)])
        if node2.type=='ovsswitch':
            node2.get_instance().exec_run(["sh","-c","ovs-vsctl add-port br {}".format(self.intf_name2)])
        idx1 = ipr.link_lookup(ifname='veth1')[0]
        idx2 = ipr.link_lookup(ifname='veth2')[0]
        # print(idx1,idx2,addr1,addr2,node1.name,node2.name,self.intf_name1,self.intf_name2)
        ipr.link('set', index=idx1, address=addr1, ifname=self.intf_name1,
                 net_ns_fd=node1.name, state='up')
        ipr.link('set', index=idx2, address=addr2, ifname=self.intf_name2,
                 net_ns_fd=node2.name, state='up')
        try:
            node1.get_instance().exec_run(["sh","-c","tc qdisc add dev {} root handle 1: htb ".format(self.intf_name1)])
            node1.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format(self.intf_name1)])
            node1.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format(self.intf_name1)])
            node2.get_instance().exec_run(["sh","-c","tc qdisc add dev {} root handle 1: htb ".format(self.intf_name2)])
            node2.get_instance().exec_run(["sh","-c","tc class add dev {} parent 1: classid 1:1 htb rate 50mbit".format(self.intf_name2)])
            node2.get_instance().exec_run(["sh","-c","tc filter add dev {} parent 1: protocol ip u32 match ip dst 0.0.0.0/0 flowid 1:1".format(self.intf_name2)])
            print('节点 {}和{}之间的链路属性配置初始化成功'.format(node1.name, node2.name))
        except Exception as e:
            print(type(e),e)
            print('节点 {}和{}之间的链路属性配置初始化错误'.format(node1.name, node2.name))
        print("节点 {}和{}之间的连接已建立".format(node1.name, node2.name))

    def del_link(self):
        self.intf1.delete()
        self.intf2.delete()
        self.intf1 = None
        self.intf2 = None
        print("节点 {}和节点 {}之间的连接已删除".format(self.nodes[0].name, self.nodes[1].name))

