class Pin:
    OUT = "OUT"

    def __init__(self, *args):
        self.args = args

    def on(self):
        print(f"STUB: machine.Pin{self.args}.on()")

    def off(self):
        print(f"STUB: machine.Pin.{self.args}.off()")


class PWM:
    def __init__(self, *a):
        self.args = a

    def freq(self, *a):
        print(f"STUB: machine.PWM{self.args},freq({a})")

    def deinit(self):
        print(f"STUB: machine.PWM{self.args},deinit()")


class UART:
    def __init__(self, *a):
        pass

    def init(self, **kwargs):
        print(f"STUB: machine.UART,init({kwargs})")

    def deinit(self):
        print(f"STUB: machine.UART,deinit()")
