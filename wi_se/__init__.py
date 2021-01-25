import esp
import machine
import uasyncio as asyncio
import uos as os
from machine import Pin

from . import conf
from . import network
from .server import Server

VERSION = '0.1'

boot0_pin = Pin(0, Pin.IN, Pin.PULL_UP)


def main():
    # He protec, he attac, but most importantly he overcloc
    machine.freq(160000000)

    # Do not unbind REPL if BOOT0/FLASH button is pressed
    if conf.unbind_repl and boot0_pin.value():
        print("Unbinding REPL")
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
