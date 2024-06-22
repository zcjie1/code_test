import docker
import sys
import os
import re
import pty
import time
import subprocess
from link import Intf
from utils import BaseString



class Node(object):
    portBase = 0

    def __init__(self, name, nodetype, **params):
        self.name = name
        self.type = nodetype
        self.intfs = {}
        self.ports = {}
        self.nameToIntf = {}  # dict of interface names to Intfs
        self.params = params
        self.mac = params.get('mac', None)
        self.ip = params.get('ip', None)
        # docker.from_env().containers.run('self.type', detach=True, tty=True, network=None,
                                         # privileged=True,
                                         # labels={'type': self.type},
                                         # name=self.name)  # 创建容器实例
        configpath = params.get('configpath')
        if configpath:
            host_dir,container_dir = configpath.split(':')
            print(host_dir,container_dir)
            if not os.path.exists(host_dir):
                print(f"The host directory '{host_dir}' does not exist. Aborting container creation.")
            else:
                if nodetype == 'nginx':
                    docker.from_env().containers.run('nginx2', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                )  # 创建容器实例
                elif nodetype == 'quagga':
                    docker.from_env().containers.run('quagga', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                )  # 创建容器实例
                elif nodetype == 'ryu':
                    docker.from_env().containers.run('osrg/ryu-book', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                )  # 创建容器实例
                elif nodetype=='ubuntu':
                    docker.from_env().containers.run('ubuntu', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='ubuntu-vim-ipv6':
                    docker.from_env().containers.run('ubuntu-vim-ipv6', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='centos':
                    docker.from_env().containers.run('centos', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='alpine':
                    docker.from_env().containers.run('alpine', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='redis':
                    docker.from_env().containers.run('redis', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='mysql':
                    docker.from_env().containers.run('mysql', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例 
                elif nodetype=='FRRouting':
                    docker.from_env().containers.run('frrouting/frr', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='ubuntu-gedit':
                    docker.from_env().containers.run('ubuntu-gedit', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    )  # 创建容器实例
                elif nodetype=='ubuntu-vlc':
                    docker.from_env().containers.run('ubuntu-vlc', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    environment={'DISPLAY': ':0'},#设置环境变量display
                                                    )  # 创建容器实例
                elif nodetype=='station':
                    docker.from_env().containers.run('station', detach=True, tty=True, network=None,
                                                    privileged=True,
                                                    labels={'type': self.type},
                                                    name=self.name,
                                                    volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                    environment={'DISPLAY': ':0'},#设置环境变量display
                                                    )  # 创建容器实例
                else:
                    print(nodetype)
                    print('未能创建相应类型节点容器，自动创建为ubuntu类型的节点容器')
                    docker.from_env().containers.run('ubuntu', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                )  # 创建容器实例
                    pass
        else :
            if nodetype == 'nginx':
                docker.from_env().containers.run('nginx2', detach=True, tty=True, network=None,
                                            privileged=True,
                                            labels={'type': self.type},
                                            name=self.name,
                                            )  # 创建容器实例
            elif nodetype == 'quagga':
                docker.from_env().containers.run('quagga', detach=True, tty=True, network=None,
                                            privileged=True,
                                            labels={'type': self.type},
                                            name=self.name,
                                            )  # 创建容器实例
            elif nodetype == 'ryu':
                docker.from_env().containers.run('osrg/ryu-book', detach=True, tty=True, network=None,
                                            privileged=True,
                                            labels={'type': self.type},
                                            name=self.name,
                                            )  # 创建容器实例
            elif nodetype=='ubuntu':
                docker.from_env().containers.run('ubuntu', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='station':
                docker.from_env().containers.run('station', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='ubuntu-vim-ipv6':
                docker.from_env().containers.run('ubuntu-vim-ipv6', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='centos':
                docker.from_env().containers.run('centos', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='alpine':
                docker.from_env().containers.run('alpine', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='FRRouting':
                docker.from_env().containers.run('frrouting/frr', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='redis':
                docker.from_env().containers.run('redis', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='mysql':
                docker.from_env().containers.run('mysql', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例 
            elif nodetype=='ubuntu-vlc':
                docker.from_env().containers.run('ubuntu-vlc', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            elif nodetype=='ubuntu-gedit':
                docker.from_env().containers.run('ubuntu-gedit', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
            else:
                print(nodetype)
                print('未能创建相应类型节点容器，自动创建为ubuntu类型的节点容器')
                docker.from_env().containers.run('ubuntu', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                )  # 创建容器实例
                pass
        self.create_netns()
        self.get_instance().exec_run(["sh","-c","echo 'net.ipv6.conf.all.disable_ipv6 = 0' >> /etc/sysctl.conf && sysctl -p"])
        print("节点 {}已创建".format(self.name))

    def get_instance(self):
        return docker.from_env().containers.get(self.name)

    # 创建相应的namespace

    def create_netns(self):
        node_name = self.name
        if not (os.path.exists("/var/run/netns/" + node_name)):
            # print("path doesn't exists")
            while True:
                try:
                    container = self.get_instance()
                    if container.attrs['State']['Running']:
                        # print("成功创建")
                        break
                    else:
                        time.sleep(1)
                        print("这不就else了")

                except:
                    print("等待节点 {}的namespace创建 ...".format(node_name))
                    sys.stdout.flush()
                    time.sleep(1)
                    pass

            pid = container.attrs['State']['Pid']
            # print("{} has pid={}".format(container, pid))
            if os.path.islink("/var/run/netns/" + node_name):
                os.unlink("/var/run/netns/" + node_name)
            os.symlink("/proc/{}/ns/net".format(pid),
                       "/var/run/netns/" + node_name)

    def newPort(self):
        if len(self.ports) > 0:
            return max(self.ports.values()) + 1
        return self.portBase

    def addIntf(self, intf, port=None):
        if port is None:
            port = self.newPort()
        self.intfs[port] = intf
        self.ports[intf] = port
        self.nameToIntf[intf.name] = intf

    def delIntf(self, intf):
        port = self.ports.get(intf)
        if port is not None:
            del self.intfs[port]
            del self.ports[intf]
            del self.nameToIntf[intf.name]

    def defaultIntf(self):
        ports = self.intfs.keys()
        if ports:
            return self.intfs[min(ports)]

    def intf(self, intf=None):
        if not intf:
            return self.defaultIntf()
        elif isinstance(intf, BaseString):
            return self.nameToIntf[intf]
        else:
            return intf

    def intfList(self):
        return [self.intfs[p] for p in sorted(self.intfs.keys())]

    def deleteIntfs(self):
        for intf in list(self.intfs.values()):
            intf.delete()

    def remove_node(self):
        try:
            instance = self.get_instance()
            # 删除容器
            instance.remove(force=True)

            print("节点{}已成功删除".format(self.name))
        except docker.errors.NotFound:
            print("找不到节点{}".format(self.name))
        except Exception as e:
            print("删除节点{}时出现错误:{}".format(self.name, e))

    def setMAC(self, mac, intf=None):
        return self.intf(intf).setMAC(mac)

    def setIP(self, ip, prefixLen=8, intf=None, **kwargs):
        return self.intf(intf).setIP(ip, prefixLen, **kwargs)

    def IP(self, intf=None):
        return self.intf(intf).IP()

    def MAC(self, intf=None):
        return self.intf(intf).MAC()

    def setParam(self, results, method, **param):
        name, value = list(param.items())[0]
        if value is None:
            return None
        f = getattr(self, method, None)
        if not f:
            return None
        if isinstance(value, list):
            result = f(*value)
        elif isinstance(value, dict):
            result = f(**value)
        else:
            result = f(value)
        results[name] = result
        return result

    def config(self, mac=None, ip=None, **_params):
        r = {}
        self.setParam(r, 'setMAC', mac=mac)
        self.setParam(r, 'setIP', ip=ip)
        return r

    def open_xterm(self):
        if self.type == 'alpine':
            try:
                command = f'docker exec -it {self.name} sh -c "echo \'export PS1=\\\"{self.name}>\\\"\' >> /root/.profile && exec sh"'
                subprocess.Popen(['xterm', '-T', self.name, '-e', 'sh', '-c', command])
                # print(1)
            except Exception as e:
                print(e)
        else:
            try:
                command = f'docker exec -it {self.name} bash -c "echo \'export PS1=\\\"{self.name}>\\\"\' >> /root/.bashrc && exec bash"'
                subprocess.Popen(['xterm', '-T', self.name, '-e', 'bash', '-c', command])
                # print(2)
            except Exception as e:
                print(e)

    def cmd(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], list):
            cmd = args[0]
            # Allow sendCmd( cmd, arg1, arg2... )
        elif len(args) > 0:
            cmd = args
        else:
            cmd = args
            # Convert to string
        if not isinstance(cmd, str):
            cmd = ' '.join([str(c) for c in cmd])
        instance = self.get_instance()
        exec_id = instance.exec_run(cmd, tty=True)

        output = exec_id.output.decode('utf-8').strip()
        return output

    def __str__(self):
        return self.name


class Host(Node):
    pass


class Switch(Node):
    portBase = 1  # Switches start with port 1 in OpenFlow
    dpidLen = 16  # digits in dpid passed to switch

    def __init__(self, name, switch_type=None, dpid=None, opts='', listenPort=None, **params):
        # Node.__init__(self, name, switch_type, **params)
        self.name = name
        self.type = switch_type
        self.intfs = {}
        self.ports = {}
        self.nameToIntf = {}  # dict of interface names to Intfs
        self.params = params
        self.mac = params.get('mac', None)
        self.ip = params.get('ip', None)
        # docker.from_env().containers.run('self.type', detach=True, tty=True, network=None,
                                         # privileged=True,
                                         # labels={'type': self.type},
                                         # name=self.name)  # 创建容器实例
        configpath = params.get('configpath')
        if configpath:
            host_dir,container_dir = configpath.split(':')
            print(host_dir,container_dir)
            if not os.path.exists(host_dir):
                print(f"The host directory '{host_dir}' does not exist. Aborting container creation.")
            else:
                docker.from_env().containers.run('ovsswitch', detach=True, tty=True, network=None,
                                                privileged=True,
                                                labels={'type': self.type},
                                                name=self.name,
                                                volumes={host_dir: {'bind': container_dir, 'mode': 'rw'}},  # 设置挂载
                                                command = ["sh","-c","export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovs-vswitchd start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovsdb-server start && ovs-vsctl add-br br && /bin/bash && ovs-ofctl add-flow br priority=0,actions=NORMAL &&./node_exporter-1.0.1.linux-amd64/node_exporter"]
                                                )  # 创建容器实例
        else:
            docker.from_env().containers.run('ovsswitch', detach=True, tty=True, network=None,
                                            privileged=True,
                                            labels={'type': self.type},
                                            name=self.name,
                                            # command = ["sh","-c","export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovs-vswitchd start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovsdb-server start && ovs-vsctl add-br br && /bin/bash && ovs-ofctl add-flow br priority=0,actions=NORMAL &&/bin/bash && ./node_exporter-1.0.1.linux-amd64/node_exporter"]
                                            # command=[
                                            #     "sh", "-c",
                                            #     "export PATH=$PATH:/usr/share/openvswitch/scripts && "+
                                            #     "ovs-ctl start && "+
                                            #     # 根据实际情况调整以下命令顺序和内容
                                            #     # "export PATH=$PATH:/usr/share/openvswitch/scripts && "
                                            #     "ovs-ctl --no-ovs-vswitchd start && "+
                                            #     # "export PATH=$PATH:/usr/share/openvswitch/scripts && "
                                            #     "ovs-ctl --no-ovsdb-server start && "+
                                            #     "ovs-vsctl add-br br && "+
                                            #     # "/bin/bash &&"
                                            #     # "ovs-ofctl add-flow br priority=0,actions=NORMAL &&"
                                            #     "./node_exporter-1.0.1.linux-amd64/node_exporter"
                                            # ]
                                            )  # 创建容器实例
        # command = ["sh","-c","export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovs-vswitchd start && export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl --no-ovsdb-server start && ovs-vsctl add-br br"]
        self.get_instance().exec_run(["sh","-c","export PATH=$PATH:/usr/share/openvswitch/scripts && ovs-ctl start && ovs-ctl --no-ovs-vswitchd start && ovs-ctl --no-ovsdb-server start && ovs-vsctl add-br br && ./node_exporter-1.0.1.linux-amd64/node_exporter"])
        self.create_netns()
        print("节点 {}已创建".format(self.name))
        self.dpid = self.defaultDpid(dpid)
        self.opts = opts
        self.listenPort = listenPort

    def defaultDpid(self, dpid=None):
        if dpid:
            # Remove any colons and make sure it's a good hex number
            dpid = dpid.replace(':', '')
            assert len(dpid) <= self.dpidLen and int(dpid, 16) >= 0
        else:
            # Use hex of the first number in the switch name
            nums = re.findall(r'\d+', self.name)
            if nums:
                dpid = hex(int(nums[0]))[2:]
            else:
                raise Exception('Unable to derive default datapath ID - '
                                'please either specify a dpid or use a '
                                'canonical switch name such as s23.')
        return '0' * (self.dpidLen - len(dpid)) + dpid


class OVSSwitch(Switch):
    def __init__(self, name, switch_type=None, failMode='secure', datapath='kernel',
                 inband=False, protocols=None,
                 reconnectms=1000, stp=False, batch=False, **params):
        Switch.__init__(self, name, switch_type, **params)
        self.bridge = []
        self.failMode = failMode
        self.datapath = datapath
        self.inband = inband
        self.protocols = protocols
        self.reconnectms = reconnectms
        self.stp = stp
        self._uuids = []  # controller UUIDs
        self.batch = batch
        self.commands = []  # saved commands for batch startup
        self.start()

    def dpctl(self, *args):
        command = 'ovs-ofctl ' + ' '.join(map(str, args))
        return self.cmd(command)

    def vsctl(self, *args):
        command = 'ovs-vsctl ' + ' '.join(map(str, args))
        return self.cmd(command)

    def intfOpts(self, intf):
        opts = ''
        opts += ' ofport_request=%s' % self.ports[intf]
        if isinstance(intf, Intf):
            intf1, intf2 = intf.link.intf1, intf.link.intf2
            peer = intf1 if intf1 != intf else intf2
            opts += ' type=patch options:peer=%s' % peer
        return '' if not opts else ' -- set Interface %s' % intf + opts

    def bridgeOpts(self):
        opts = (' other_config:datapath-id=%s' % self.dpid +
                ' fail_mode=%s' % self.failMode)
        if not self.inband:
            opts += ' other-config:disable-in-band=true'
        if self.datapath == 'user':
            opts += ' datapath_type=netdev'
        if self.protocols:
            opts += ' protocols=%s' % self.protocols
        if self.stp and self.failMode == 'standalone':
            opts += ' stp_enable=true'
        opts += ' other-config:dp-desc=%s' % self.name
        return opts

    def start(self):
        int(self.dpid, 16)
        intfs = ''.join(' -- add-port %s %s' % (self, intf) +
                        self.intfOpts(intf)
                        for intf in self.intfList()
                        if self.ports[intf] and not intf.IP())
        cargs = ' -- --if-exists del-br %s' % self
        self.vsctl(cargs +
                   ' -- add-br %s' % self + self.bridgeOpts() + intfs)

class server(Node):
    pass

class nginx(Node):
    pass

class router(Node):
    pass

class quagga(Node):
    pass

class controller(Node):
    pass

class ryu(Node):
    pass
