import esp
import uasyncio as asyncio
import uos as os

from . import conf
from . import network
from .server import Server

VERSION = '0.1'


def main():
    if conf.unbind_repl:
        # Disable debug messages
        esp.osdebug(None)
        # Unbind REPL
        os.dupterm(None, 1)

    loop = asyncio.get_event_loop()

    wlan = network.up()
    loop.create_task(network.ensure_up())

    # noinspection PyShadowingNames
    server = Server()
    loop.create_task(server.serve())

    loop.run_forever()
