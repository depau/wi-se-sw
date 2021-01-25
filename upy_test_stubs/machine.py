class Pin:
    OUT = "OUT"
    IN = "IN"

    PULL_UP = "PULL_UP"
    PULL_DOWN = "PULL_DOWN"

    def __init__(self, *args):
        self.args = args

    def on(self):
        print(f"STUB: machine.Pin{self.args}.on()")

    def off(self):
        print(f"STUB: machine.Pin.{self.args}.off()")

    def value(self, *a):
        print(f"STUB: machine.Pin.{self.args}.value{a}")
        if self.args[0] == 0:
            return 1
        return 0


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


def freq(new_clock):
    print("OVERCLOCCCKKKK OMH SUCH SPEED VERY FAST {}".format(new_clock))
