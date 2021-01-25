from machine import Pin

from . import conf as cfg

if cfg.board_type == "custom":
    wifi = Pin(cfg.led_wifi, Pin.OUT)
    status = Pin(cfg.led_status, Pin.OUT)
    tx = Pin(cfg.led_tx, Pin.OUT)
    rx = Pin(cfg.led_rx, Pin.OUT)

elif cfg.board_type in ("wi-se-rpi-v0.1", "wi-se-opi4-v0.1"):
    wifi = Pin(14, Pin.OUT)
    status = Pin(3, Pin.OUT)
    tx = Pin(5, Pin.OUT)
    rx = Pin(4, Pin.OUT)

elif cfg.board_type == "wi-se-rewirable-v0.1":
    wifi = Pin(14, Pin.OUT)
    status = Pin(5, Pin.OUT)
    tx = Pin(3, Pin.OUT)
    rx = Pin(4, Pin.OUT)

elif cfg.board_type == "stub":
    wifi = Pin("Wi-Fi")
    status = Pin("Status")
    tx = Pin("TXD")
    rx = Pin("RXD")

else:
    raise RuntimeError("Unknown board type: '{}'".format(cfg.board_type))
