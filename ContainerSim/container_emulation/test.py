from pyroute2 import NDB, IPRoute

def delete(node_name, name):
    with NDB(log='stdout') as ndb:
        ndb.sources.add(netns=node_name)
        interface = ndb.interfaces[{'target': node_name, 'ifname': name}]
        print(interface)
        interface.remove()
        interface.commit()

delete('host2', 'tohost1')
