import sys


Python3 = sys.version_info[0] == 3
BaseString = str if Python3 else getattr(str, '__base__')
Encoding = 'utf-8' if Python3 else None


def ipNum(w, x, y, z):
    """Generate unsigned int from components of IP address
       returns: w << 24 | x << 16 | y << 8 | z"""
    return (w << 24) | (x << 16) | (y << 8) | z


def ipStr(ip):
    """Generate IP address string from an unsigned int.
       ip: unsigned int of form w << 24 | x << 16 | y << 8 | z
       returns: ip address string w.x.y.z"""
    w = (ip >> 24) & 0xff
    x = (ip >> 16) & 0xff
    y = (ip >> 8) & 0xff
    z = ip & 0xff
    return "%i.%i.%i.%i" % (w, x, y, z)


def ipAdd(i, prefixLen=8, ipBaseNum=0x0a000000):
    """Return IP address string from ints
       i: int to be added to ipbase
       prefixLen: optional IP prefix length
       ipBaseNum: option base IP address as int
       returns IP address as string"""
    imax = 0xffffffff >> prefixLen
    assert i <= imax, 'Not enough IP addresses in the subnet'
    mask = 0xffffffff ^ imax
    ipnum = (ipBaseNum & mask) + i
    return ipStr(ipnum)

def ipParse(ip):
    "Parse an IP address and return an unsigned int."
    args = [int(arg) for arg in ip.split('.')]
    while len(args) < 4:
        args.insert(len(args) - 1, 0)
    return ipNum(*args)


def netParse(ipstr):
    """Parse an IP network specification, returning
       address and prefix len as unsigned ints"""
    prefixLen = 0
    if '/' in ipstr:
        ip, pf = ipstr.split('/')
        prefixLen = int(pf)
    # if no prefix is specified, set the prefix to 24
    else:
        ip = ipstr
        prefixLen = 24
    return ipParse(ip), prefixLen