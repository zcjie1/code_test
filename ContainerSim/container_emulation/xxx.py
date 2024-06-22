from os import getpid
from subprocess import check_output as co


def get_hwsim_list():
    return 'find /sys/kernel/debug/ieee80211 -name hwsim | grep \'[0-9]\' | cut -d/ -f 6 | sort '% getpid()


def get_intf_list(cmd):
    'Gets all phys after starting the wireless module'
    phy = co(cmd, shell=True).decode('utf-8').split("\n")
    phy.pop()
    phy.sort(key=len, reverse=False)
    return phy

xxx = get_intf_list(get_hwsim_list())
if xxx:
    print(xxx)