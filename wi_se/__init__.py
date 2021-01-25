VERSION = '0.1'

import asyncio

import esp

from . import network as net


def main():
    # Disable debug messages
    esp.osdebug(None)

    loop = asyncio.get_event_loop()

    wlan = net.up()
    loop.create_task(net.ensure_up())
