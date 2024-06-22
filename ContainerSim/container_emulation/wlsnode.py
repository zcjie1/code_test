from node import Node
from utils import BaseString
import docker


class Node_wifi(Node):

    portBase = 0

    def __init__(self, name, nodetype, **params):
        super().__init__(name, nodetype, **params)
        if 'position' in params:
            self.position = params.get('position')
        if 'mode' in params:
            self.mode = params.get('mode')
        if 'essid' in params:
            self.essid = params.get('essid')
        self.wintfs = {}
        self.wports = {}

    def get_wlan(self, intf):
        return self.params['wlan'].index(intf)

    def getNameToWintf(self, intf):
        wlan = self.get_wlan(intf) if isinstance(intf, BaseString) else 0
        return self.wintfs[wlan]

    def setIP(self, ip, prefixLen=8, intf=None, **kwargs):
        if intf in self.wintfs:
            return self.getNameToWintf(intf).setIP(ip, prefixLen, **kwargs)
        return self.intf(intf).setIP(ip, prefixLen, **kwargs)

    def newPort(self):
        if len(self.ports) > 0: return max(self.ports.values()) + 1
        return self.portBase

    def newWPort(self):
        if len(self.wports) > 0: return max(self.wports.values()) + 1
        return self.portBase

    def addWAttr(self, intf, port=None):
        if port is None: port = self.newWPort()

        self.wintfs[port] = intf
        self.wports[intf] = port
        self.nameToIntf[intf.name] = intf

    def addWIntf(self, intf, port=None):
        if port is None: port = self.newPort()
        self.intfs[port] = intf
        self.ports[intf] = port
        self.nameToIntf[intf.name] = intf

    def remove_node(self):
        try:
            instance = self.get_instance()
            if self.wintfs:
                for _, value in self.wintfs.items():
                    value.del_self()
            # 删除容器
            instance.remove(force=True)

            print("节点{}已成功删除".format(self.name))
        except docker.errors.NotFound:
            print("找不到节点{}".format(self.name))
        except Exception as e:
            print("删除节点{}时出现错误:{}".format(self.name, e))


class Station(Node_wifi):
    pass
