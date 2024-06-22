from link import Intf, Link
from wmediumdConnector import DynamicIntfRef, \
    WStarter, SNRLink, w_pos, w_cst, w_server, ERRPROBLink, \
    wmediumd_mode, w_txpower, w_gain, w_height, w_medium
import socket
from pyroute2 import NDB


class IntfWireless(Intf):
    dist = 0
    noise = 0
    medium_id = 0
    eqLoss = '(dist * 2) / 1000'
    eqDelay = '(dist / 10) + 1'
    eqLatency = '(dist / 10)/2'
    eqBw = ' * (1.01 ** -dist)'

    def __init__(self, name, node=None, port=None, link=None,
                  mac=None, **params):
        self.node = node
        self.name = name
        self.link = link
        self.mac = mac
        self.ip, self.prefixLen = None, None
        if self.name == 'lo':
            self.ip = '127.0.0.1'
            self.prefixLen = 8
        if node:
            node.addWIntf(self, port=port)
        self.params = params
        self.config(**params)

    def iwdev_cmd(self, *args):
        return self.cmd('iw dev', *args)

    def set_dev_type(self, *args):
        return self.iwdev_cmd('{} set type {}'.format(self.name, *args))

    def add_dev_type(self, new_name, type):
        return self.iwdev_cmd('{} interface add {} type {}'.format(
            self.name, new_name, type))

    def set_bitrates(self, *args):
        return self.iwdev_cmd('{} set bitrates'.format(self.name), *args)

    def join_ibss(self, *args):
        return self.iwdev_cmd('{} ibss join'.format(self.name), *args)

    def ibss_leave(self):
        return self.iwdev_cmd('{} ibss leave'.format(self.name))

    def mesh_join(self, ssid, *args):
        return self.iwdev_cmd('{} mesh join'.format(self.name), ssid, 'freq', *args)


class AdhocIntf(Intf):
    def __init__(self, name, node=None, port=None,
                  mac=None, essid=None, **params):
        self.node = node
        self.name = name
        self.port = port
        self.mac = mac
        self.essid = essid
        self.ip, self.prefixLen = None, None
        self.ipv6 ,self.prefixLenv6 =None,None
        if self.name == 'lo':
            self.ip = '127.0.0.1'
            self.prefixLen = 8
        self.params = params
        self.config(**params)
        self.set_dev_type('ibss')
        self.join_ibss(self.essid, '2432')
        self.node.addWIntf(self, port=self.port)
        self.node.addWAttr(self, port=self.port)
        print(self.name)

    def ifconfig_down(self):
        return self.cmd('ifconfig {} down'.format(self.name))
    

    def ifconfig_up(self):
        return self.cmd('ifconfig {} up'.format(self.name))
    
    def iwdev_cmd(self, *args):
        return self.cmd('iw dev', *args)

    def set_dev_type(self, *args):
        return self.iwdev_cmd('{} set type {}'.format(self.name, *args))

    def add_dev_type(self, new_name, type):
        return self.iwdev_cmd('{} interface add {} type {}'.format(
            self.name, new_name, type))

    def set_bitrates(self, *args):
        return self.iwdev_cmd('{} set bitrates'.format(self.name), *args)

    def join_ibss(self, *args):
        return self.iwdev_cmd('{} ibss join'.format(self.name), *args)

    def ibss_leave(self):
        return self.iwdev_cmd('{} ibss leave'.format(self.name))

    def mesh_join(self, ssid, *args):
        return self.iwdev_cmd('{} mesh join'.format(self.name), ssid, 'freq', *args)

    def del_self(self):
        return self.iwdev_cmd('{} del'.format(self.name))

    def setMAC(self, macstr):
        if not macstr:
            print("未设置mac地址")
            return
        self.mac = macstr
        self.cmd('ip link set dev {} address {}'.format(self.name, self.mac))
        print("节点 {}的{}端口的mac地址：{}已配置".format(self.node.name, self.name, self.mac))

    # def setIP(self, ipstr, prefixLen=None):
    #     if not ipstr:
    #         print("未设置ip地址")
    #         return
    #     if self.ip:
    #         self.cmd('ip address del {}'.format(self.ip))
    #     if '/' in ipstr:
    #         self.ip, self.prefixLen = ipstr.split('/')
    #     else:
    #         if prefixLen is None:
    #             raise Exception('No prefix length set for IP address %s'
    #                             % (ipstr,))
    #         self.ip, self.prefixLen = ipstr, prefixLen
    #     print('adhoc','ip address add {}/{} dev {}'.format(self.ip, self.prefixLen, self.name))
    #     result=self.cmd('ip address add {}/{} dev {}'.format(self.ip, self.prefixLen, self.name))
    #     print("节点 {}的{}端口的ip地址：{}/{}已配置".format(self.node.name, self.name, self.ip, self.prefixLen))
    #     return result
    
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


class AdhocwlsLink(object):
    def __init__(self, essid=None):
        self.essid = essid
        self.wintf = []

    def add_wintf(self, wintf):
        self.wintf.append(wintf)

    def del_wintf(self, wintf):
        self.wintf.remove(wintf)



