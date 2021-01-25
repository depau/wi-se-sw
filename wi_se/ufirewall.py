from . import conf


def parse_ip4(ipaddr: str) -> int:
    chunks = [int(i) for i in reversed(ipaddr.split('.'))]
    ret = 0
    for i in range(4):
        ret |= chunks[i] << (8 * i)
    return ret


# _private_nets = (
#     # (ip address, netmask)
#     (parse_ip4('192.168.0.0'), parse_ip4('255.255.0.0')),
#     (parse_ip4('10.0.0.0'), parse_ip4('255.0.0.0')),
#     (parse_ip4('172.16.0.0'), parse_ip4('255.240.0.0')),
# )

# Pre-computed
_private_nets = (
    (3232235520, 4294901760),
    (167772160, 4278190080),
    (2886729728, 4293918720)
)


def is_allowed(ipaddr) -> bool:
    if not conf.private_nets_only:
        return True

    ip4 = parse_ip4(ipaddr)
    for net, mask in _private_nets:
        if (ip4 & mask) == (net & mask):
            return True

    return False
