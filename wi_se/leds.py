from machine import Pin, Signal

from . import conf as cfg

if cfg.board_type == "custom":
    wifi = Signal(Pin(cfg.led_wifi, Pin.OUT), invert=getattr(cfg, "led_wifi_invert", False))
    status = Signal(Pin(cfg.led_status, Pin.OUT), invert=getattr(cfg, "led_status_invert", False))
    tx = Signal(Pin(cfg.led_tx, Pin.OUT), invert=getattr(cfg, "led_tx_invert", False))
    rx = Signal(Pin(cfg.led_rx, Pin.OUT), invert=getattr(cfg, "led_rx_invert", False))

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

else:
    raise RuntimeError("Unknown board type: '{}'".format(cfg.board_type))
