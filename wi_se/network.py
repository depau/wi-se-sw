import time

import uasyncio as asyncio
from machine import PWM

from . import conf as cfg
from . import leds
from .constants import *

wlan = None


def up() -> network.WLAN:
    global wlan

    leds.status.on()
    leds.wifi.off()

    if cfg.wifi_mode == "sta":
        wifi_pwm = PWM(leds.wifi)
        wifi_pwm.freq(2)

        wlan = network.WLAN(network.STA_IF)
        wlan.active(True)
        #wlan.scan()
        wlan.config(dhcp_hostname=cfg.hostname)
        wlan.connect(cfg.wifi_ssid, cfg.wifi_key)

        if not wlan.isconnected():
            wlan.connect(cfg.wifi_ssid, cfg.wifi_key)
            count = 5
            while not wlan.isconnected():
                count += 1
                if count % 10 == 0:
                    wlan.connect(cfg.wifi_ssid, cfg.wifi_key)
                time.sleep(0.5)

        wifi_pwm.deinit()

    elif cfg.wifi_mode == "ap":
        wlan = network.WLAN(network.AP_IF)
        wlan.config(
            essid=cfg.wifi_ssid,
            authmode=wifi_authmodes[cfg.wifi_authmode],
            password=cfg.wifi_key,
            dhcp_hostname=cfg.hostname
        )
        wlan.active(True)

    leds.status.off()
    leds.wifi.on()

    return wlan


async def ensure_up():
    global wlan

    if cfg.wifi_mode == "ap":
        return

    while True:
        if not wlan.isconnected():
            leds.status.on()
            leds.wifi.off()

            # Turn off Wi-Fi first
            wlan.active(False)
            await asyncio.sleep(1)

            up()

        await asyncio.sleep(1)
