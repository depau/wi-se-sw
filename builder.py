#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import errno
import json
import locale
import os
import shutil
import subprocess
import sys
import traceback
from datetime import datetime
from typing import Optional

import pyjq
import yaml
from jinja2 import Environment, FileSystemLoader

try:
    from yaml import CSafeLoader as YamlLoader, CSafeDumper as YamlDumper
except ImportError:
    from yaml import SafeLoader as YamlLoader, SafeDumper as YamlDumper

BRIGHT_BLUE = '\033[34;1m'
BRIGHT_RED = '\033[31;1m'
BRIGHT_GREEN = '\033[32;1m'
RESET_COLOR = '\033[0m'

COLOR_INFO = BRIGHT_BLUE
COLOR_OK = BRIGHT_GREEN
COLOR_NOK = BRIGHT_RED

BOARD_TYPES = {
    'generic': 0,
    'wi-se-rpi-v0.1': 1,
    'wi-se-opi4-v0.1': 2,
    'wi-se-rewirable-v0.1': 3
}


# noinspection PyPep8Naming
class Extractor:
    def __init__(self, cfg: Optional[dict] = None, debug: bool = False):
        self.debug = debug
        self.cfg = cfg or {}

    def jq(self, path, default, c_string: bool = False, c_bool: bool = False):
        ret = pyjq.first(path, self.cfg, default=default)
        if ret is None:  # Default sometimes doesn't do what it's supposed to
            ret = default
        if c_bool:
            return int(bool(ret))
        if not c_string or ret is None:
            return ret
        c_str = json.dumps(ret)
        if not c_str.startswith('"') or not c_str.startswith('"'):
            raise ValueError(f"Invalid value (must be a string): {ret}")
        return c_str

    @property
    def AUTOGEN_MSG(self):
        return "AUTOMATICALLY GENERATED, DO NOT EDIT."

    @property
    def AUTOGEN_DATE(self):
        return f"Generated on {datetime.now().strftime(locale.nl_langinfo(locale.D_T_FMT))}."


# noinspection PyPep8Naming
class ConfigHeaderExtractor(Extractor):
    @property
    def ENABLE_DEBUG(self):
        if self.debug:
            return 1
        return self.jq('.debug.enable', False, c_bool=True)

    @property
    def ENABLE_BENCHMARK(self):
        return self.jq('.debug.benchmark', False, c_bool=True)

    @property
    def BOARD_TYPE(self):
        return BOARD_TYPES[self.jq('.board.type', 'generic')]

    @property
    def UART_COMM(self):
        uart_nr = self.jq('.uart.uart_comm.serial', 0)
        return f"ExtSerial{uart_nr}"

    @property
    def UART_COMM_BAUD(self):
        return self.jq('.uart.uart_comm.baud', 115200)

    @property
    def UART_COMM_CONFIG(self):
        return self.jq('.uart.uart_comm.config', '(UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1)')

    @property
    def UART_DEBUG(self):
        uart_nr = self.jq('.uart.uart_debug.serial', 0)
        return f"ExtSerial{uart_nr}"

    @property
    def UART_DEBUG_BAUD(self):
        return self.jq('.uart.uart_debug.baud', 115200)

    @property
    def UART_DEBUG_CONFIG(self):
        return self.jq('.uart.uart_debug.config', '(UART_NB_BIT_8 | UART_PARITY_NONE | UART_NB_STOP_BIT_1)')

    @property
    def WIFI_MODE(self):
        val = self.jq('.wifi.mode', 'WIFI_STA')
        if val not in ('WIFI_STA', 'WIFI_AP'):
            raise ValueError('.wifi.mode must be either WIFI_STA or WIFI_AP')
        return val

    @property
    def WIFI_SSID(self):
        val = self.jq('.wifi.ssid', None, c_string=True)
        if not val:
            return 'nullptr'
        return val

    @property
    def WIFI_PASS(self):
        return self.jq('.wifi.password', 'changemeASAP', c_string=True)

    @property
    def WIFI_HOSTNAME(self):
        return self.jq('.wifi.hostname', 'Wi_Se', c_string=True)

    @property
    def DEVICE_PRETTY_NAME(self):
        return self.jq('.name', 'Wi-Se', c_string=True)

    @property
    def WIFI_CHANNEL(self):  # AP only
        return self.jq('.wifi.ap.channel', 6)

    @property
    def WIFI_HIDE_SSID(self):
        return self.jq('.wifi.ap.hide_ssid', False, c_bool=True)

    @property
    def WIFI_MAX_DEVICES(self):
        return self.jq('.wifi.ap.max_clients', 6)

    @property
    def OTA_ENABLE(self):
        return self.jq('.ota.enable', False, c_bool=True)

    @property
    def OTA_PASSWORD(self):
        ret = self.jq('.ota.password', '', c_string=True)
        if ret == '""' and self.OTA_ENABLE:
            raise ValueError("OTA enabled but empty password")
        return ret

    @property
    def HTTP_LISTEN_PORT(self):
        return self.jq('.http.port', 80)

    @property
    def HTTP_AUTH_ENABLE(self):
        return self.jq('.http.auth.enable', False, c_bool=True)

    @property
    def HTTP_AUTH_USER(self):
        return self.jq('.http.auth.user', "", c_string=True)

    @property
    def HTTP_AUTH_PASS(self):
        return self.jq('.http.auth.password', "", c_string=True)

    @property
    def WS_MAX_CLIENTS(self):
        return self.jq('.ws.max_clients', 3)

    @property
    def WS_PING_INTERVAL(self):
        return self.jq('.ws.ping_interval', 300)

    @property
    def TTYD_WEB_CONFIG(self):
        cfg = self.jq('.ttyd.web_config', None) or {"disableLeaveAlert": True}
        if not isinstance(cfg, dict):
            raise ValueError(".ttyd.web_config must be a dictionary")
        return json.dumps(json.dumps(cfg))

    @property
    def LED_WIFI(self):
        return self.jq('.board.leds.wifi', 5)

    @property
    def LED_STATUS(self):
        return self.jq('.board.leds.status', 13)

    @property
    def LED_TX(self):
        return self.jq('.board.leds.tx', 14)

    @property
    def LED_RX(self):
        return self.jq('.board.leds.rx', 12)

    @property
    def LED_ON_TIME(self):
        return self.jq('.board.leds.on_time', 15)

    @property
    def LED_OFF_TIME(self):
        return self.jq('.board.leds.off_time', 15)

    @property
    def UART_RX_BUF_SIZE(self):
        return self.jq('.uart.advanced.rx_buf_size', 10240)

    @property
    def UART_RX_SOFT_MIN(self):
        return self.jq('.uart.advanced.rx_soft_min', '(WS_SEND_BUF_SIZE * 3 / 2)')

    @property
    def UART_BUFFER_BELOW_SOFT_MIN_DYNAMIC_DELAY(self):
        return self.jq('.uart.advanced.buffer_below_soft_min_dynamic_delay',
                       '(std::min((int) (1000L * WS_SEND_BUF_SIZE * 8L * 2 / 3 / uartBaudRate), 5))')

    @property
    def UART_AUTOBAUD_TIMEOUT_MILLIS(self):
        return self.jq('.uart.advanced.autobaud.timeout', 10000)

    @property
    def UART_AUTOBAUD_ATTEMPT_INTERVAL(self):
        return self.jq('.uart.advanced.autobaud.attempt_interval', 100)

    @property
    def UART_SW_FLOW_CONTROL(self):
        return self.jq('.uart.advanced.flow_control.enable', True, c_bool=True)

    @property
    def UART_SW_FLOW_CONTROL_LOW_WATERMARK(self):
        return self.jq('.uart.advanced.flow_control.low_watermark', 'UART_RX_SOFT_MIN + 1')

    @property
    def UART_SW_FLOW_CONTROL_HIGH_WATERMARK(self):
        return self.jq('.uart.advanced.flow_control.high_watermark', 'WS_SEND_BUF_SIZE - 1')

    @property
    def UART_SW_LOCAL_FLOW_CONTROL_STOP_MAX_MS(self):
        return self.jq('.uart.advanced.flow_control.local_max_stop_time', 500)

    @property
    def WS_SEND_BUF_SIZE(self):
        return self.jq('.ws.advanced.buffer_size', 1536)

    @property
    def HEAP_FREE_LOW_WATERMARK(self):
        return self.jq('.uart.advanced.flow_control.heap_free_low_watermark', 4096)

    @property
    def HEAP_FREE_HIGH_WATERMARK(self):
        return self.jq('.uart.advanced.flow_control.heap_free_high_watermark', 10240)

    @property
    def HEAP_CAUSED_WS_FLOW_CTL_STOP_MAX_MS(self):
        return self.jq('.uart.advanced.flow_control.ws_max_stop_time', 500)


class PlatformIOExtractor(Extractor):
    @property
    def atomic_ota(self):
        return self.jq('.ota.atomic', True)

    @property
    def cpu_freq(self):
        freq = self.jq('.board.cpu_freq', 160000000) or 160000000
        return str(freq) + 'L'

    @property
    def upload_protocol(self):
        ret = self.jq('.upload.method', 'serial')
        if ret not in ('serial', 'ota'):
            raise ValueError(".upload.method must be one of serial, ota")
        return ret

    @property
    def serial_port(self):
        ret = self.jq('.upload.serial.port', None)
        if self.upload_protocol == 'serial' and not ret:
            raise ValueError(".upload.method is serial but .upload.serial.port not provided")
        return json.dumps(ret)

    @property
    def serial_baud(self):
        return self.jq('.upload.serial.baud', 921600)

    @property
    def ota_address(self):
        ret = self.jq('.upload.ota.address', None)
        if self.upload_protocol == 'ota' and not ret:
            raise ValueError(".upload.method is ota but .upload.ota.address not provided")
        return json.dumps(ret)

    @property
    def ota_host_port(self):
        return self.jq('.upload.ota.host_port', 8266)

    @property
    def ota_password(self):
        return self.jq('.upload.ota.password', None)

    @property
    def local_version(self):
        commit = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], cwd=git_toplevel_dir(), capture_output=True, encoding='utf-8',
                                check=True).stdout.strip()
        try:
            subprocess.run(['git', 'diff', '--quiet'], cwd=git_toplevel_dir(), check=True)
            dirty = False
        except subprocess.CalledProcessError:
            dirty = True
        # Double "dumps" since one set of quotes is for pio.ini, the second needs to end up in the header
        return json.dumps(json.dumps(f"-{commit}{'-dirty' if dirty else ''}"))


class Builder:
    def __init__(self, config: str, debug: bool = False):
        self.config_file = config
        self.config_name = os.path.splitext(os.path.basename(config))[0]
        self.builder_dir = os.path.join(git_toplevel_dir(), ".builder", self.config_name)
        with open(config) as f:
            self.cfg = yaml.load(f, YamlLoader)
        self.header_extr = ConfigHeaderExtractor(self.cfg, debug)
        self.pio_extr = PlatformIOExtractor(self.cfg, debug)

    def gen_configs(self, target_dir: str) -> None:
        if not os.path.isdir(target_dir):
            raise OSError(errno.ENOTDIR, os.strerror(errno.ENOTDIR), target_dir)

        tpldir = os.path.join(git_toplevel_dir(), "builder")
        if not os.path.exists(tpldir) or not os.path.isdir(tpldir):
            raise OSError(errno.ENOENT, os.strerror(errno.ENOENT), tpldir)

        env = Environment(loader=FileSystemLoader(tpldir), trim_blocks=True)
        header_tpl = env.get_template('config.j2.h')
        pio_tpl = env.get_template('platformio.j2.ini')

        header_out = os.path.join(target_dir, "include", "config.h")
        pio_out = os.path.join(target_dir, "platformio.ini")

        print("Generate include/config.h")
        with open(header_out, "w") as f:
            header_tpl \
                .stream(cfg=self.header_extr) \
                .dump(f)

        print("Generate platformio.ini")
        with open(pio_out, "w") as f:
            pio_tpl \
                .stream(cfg=self.pio_extr) \
                .dump(f)

    def prepare_sources(self):
        os.makedirs(self.builder_dir, 0o755, exist_ok=True)

        includedirs = (
            os.path.join(git_toplevel_dir(), "include"),
            os.path.join(self.builder_dir, "include")
        )
        srcdirs = (
            os.path.join(git_toplevel_dir(), "src"),
            os.path.join(self.builder_dir, "src")
        )
        shutil.copytree(*srcdirs, symlinks=True, dirs_exist_ok=True)
        shutil.copytree(*includedirs, symlinks=True, dirs_exist_ok=True)

    def prepare(self):
        self.prepare_sources()
        self.gen_configs(self.builder_dir)

    def build(self) -> None:
        self.prepare()
        subprocess.run(['platformio', '-c', 'clion', 'run'], cwd=self.builder_dir, check=True)

    def build_and_upload(self) -> None:
        self.prepare()
        subprocess.run(['platformio', '-c', 'clion', 'run', '--target', 'upload'], cwd=self.builder_dir, check=True)


def usage():
    print(f"Usage:")
    print(f"  {sys.argv[0]} clean")
    print(f"  {sys.argv[0]} prepare [--debug] [config...]")
    print(f"  {sys.argv[0]} build [--debug] [config...]")
    print(f"  {sys.argv[0]} upload [--debug] [config...]")
    print(f"  {sys.argv[0]} devconf [--debug] [--example] [config]")
    print()
    print("Actions:")
    print("  clean       Remove build directory.")
    print("  prepare     Prepare sources for specified configs, or all configs if none is specified.")
    print("  build       Prepare and build using specified configs, or all configs if none is specified.")
    print("  upload      Prepare, build and upload to target using specified configs, or all configs if none is specified.")
    print("  devconf     Generate headers for development using specified config.")
    print()
    print("Options:")
    print("  --debug     Force-enable debug messages")
    print("  --example   Generate devconf using example config")
    raise SystemExit(0)


def nproc() -> int:
    return len(os.sched_getaffinity(0))


def git_toplevel_dir() -> str:
    # noinspection PyGlobalUndefined
    global _git_toplevel_dir
    try:
        return _git_toplevel_dir
    except NameError:
        _git_toplevel_dir = \
            subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, encoding='utf-8', check=True) \
                .stdout.strip()
    return _git_toplevel_dir


def get_console_width():
    if os.isatty(sys.stdout.fileno()):
        # noinspection PyBroadException
        try:
            width = os.get_terminal_size(sys.stdout.fileno())[0]
            if width > 0:
                return width
        except Exception:
            pass
    return 80


def banner(string, color=COLOR_INFO):
    print('=' + f" {color}{string}{RESET_COLOR} ".ljust(get_console_width() - 1, '='))


def main():
    debug = False
    jobs = nproc()
    configs = []

    args = sys.argv.copy()

    args.pop(0)
    if len(args) == 0:
        usage()

    action = args.pop(0)
    if action not in ('prepare', 'build', 'upload', 'devconf', 'clean'):
        usage()

    if action == 'clean' and len(args) > 0:
        print("Too many options", file=sys.stderr)
        usage()

    if action == 'clean':
        builder_dir = os.path.join(git_toplevel_dir(), ".builder")
        # Use subprocess so we can remove verbosely
        subprocess.run(["rm", "-Rvf", builder_dir])
        return

    while len(args) > 0:
        if args[0] == '--debug':
            debug = True
            args.pop(0)
        elif args[0] == "--example":
            configs.append(os.path.join(git_toplevel_dir(), "configs/config.yml.example"))
            args.pop(0)
        elif args[0] == "--":
            sys.argv.pop(0)
            break
        elif args[0].startswith("-"):
            print(f"Unrecognized argument: {args[0]}")
            usage()
        else:
            break

    if action == 'devconf':
        if len(configs) > 0 and len(args) > 0:
            print("Can use at most one config for devconf, and you already specified --example")
            usage()
        elif len(configs) == 0 and len(args) == 0:
            print("No config specified", file=sys.stderr)
            usage()
        elif len(configs) == 0 and len(args) > 1:
            print("Too many configs specified", file=sys.stderr)
            usage()

    while len(args) > 0:
        cfgname = args.pop(0)
        config = os.path.join(git_toplevel_dir(), "configs", cfgname + ".yml")
        if not os.path.isfile(config):
            print(f"Config file does not exist: '{config}'", file=sys.stderr)
            usage()
        configs.append(config)

    if action in ('prepare', 'build', 'upload') and len(configs) == 0:
        cfgdir = os.path.join(git_toplevel_dir(), "configs")
        for file in os.listdir(cfgdir):
            path = os.path.join(cfgdir, file)
            if file.endswith(".yml") and os.path.isfile(path):
                configs.append(path)

    for config in configs:
        builder = Builder(config, debug)
        banner(f"RUNNING ACTION {action} FOR {builder.config_name}", COLOR_INFO)

        # noinspection PyBroadException
        try:
            if action == 'build':
                builder.build()
            elif action == 'upload':
                builder.build_and_upload()
            elif action == 'devconf':
                builder.gen_configs(git_toplevel_dir())
            elif action == 'prepare':
                builder.prepare()
            banner(f"SUCCESS - {builder.config_name}", COLOR_OK)
        except Exception:
            print(COLOR_NOK, end='', file=sys.stderr)
            traceback.print_exc()
            print(RESET_COLOR, end='', file=sys.stdout)
            print(RESET_COLOR, end='', file=sys.stderr)
            banner(f"FAILED - {builder.config_name}", COLOR_NOK)

        print()


if __name__ == "__main__":
    main()
