AUTH_OPEN = "AUTH_OPEN"
AUTH_WEP = "AUTH_WEP"
AUTH_WPA_PSK = "AUTH_WPA_PSK"
AUTH_WPA2_PSK = "AUTH_WPA2_PSK"
AUTH_WPA_WPA2_PSK = "AUTH_WPA_WPA2_PSK"

STA_IF = "STA_IF"
AP_IF = "AP_IF"


class WLAN:
    def __init__(self, *args):
        print(f"STUB network.WIFI create{args}")
        self.args = args

    def active(self, active):
        print(f"STUB network.WIFI.active({active})")

    def config(self, **kwargs):
        print(f"STUB network.WIFI.config({kwargs})")

    def connect(self, ssid, psk):
        print(f"STUB network.WIFI.connect('{ssid}', '{psk}')")

    def isconnected(self):
        return True

    def scan(self):
        print(f"STUB network.WIFI.scan()")
