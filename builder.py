#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import errno
import json
import locale
import os
import shutil
import subprocess
import sys
import textwrap
import traceback
from datetime import datetime
from typing import Optional

import jq as pyjq
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
    'wi-se-rewirable-v0.1': 3,
    'esp32dev': 4
}


class SafeDict(dict):
    def get(self, key, default=None):
        val = super().get(key, default)
        return default if val is None else val


# noinspection PyPep8Naming
class Extractor:
    def __init__(self, cfg: Optional[dict] = None, debug: bool = False):
        self.debug = debug
        self.cfg = cfg or {}

    def jq(self, path, default, c_string: bool = False, c_bool: bool = False):
        ret = pyjq.first(path, self.cfg)
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
    def ADC_INPUT(self):
        return self.jq('.board.adc_input', "")

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
    def UART_COMM_TX_EN(self):
        return self.jq('.uart.uart_comm.tx_en', -1)

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
    def HTTP_CORS_ALLOW_ORIGIN(self):
        return self.jq('.http.cors_allow_origin', None, c_string=True)

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
    def TARGET_GPIO_INITS(self):
        jq = self.jq('.board.target', [])
        l = []
        for i,v in enumerate(jq):
            v = SafeDict(v)
            gpio = v['gpio']
            mode = v.get('mode','INPUT')
            init = 'TARGET_GPIO_ONINIT' if v.get('init', False) else 'TARGET_GPIO_LOCKED'
            inverted = str(v.get('inverted','false')).lower()
            l.append("{%s, %s, %s, %s, 0, &gpio_%s_dval, gpio_%s_name, gpio_%s_desc, gpio_%s_color}" % (gpio, mode, init, inverted, i, i, i, i))
        return '\\\n%s\n' % ', \\\n'.join(l) if l else ''

    @property
    def TARGET_GPIO_STRINGS(self):
        jq = self.jq('.board.target', [])
        l = []
        for i,v in enumerate(jq):
            v = SafeDict(v)
            l.append("const uint64_t gpio_%s_dval PROGMEM = %d;" % (i, v.get('dval', 0)))
            l.append("const char gpio_%s_name[] PROGMEM = \"%s\";" % (i, v.get('name', "gpio%s" % v['gpio'])))
            l.append("const char gpio_%s_desc[] PROGMEM = \"%s\";" % (i, v.get('desc', "GPIO %s" % v['gpio'])))
            l.append("const char gpio_%s_color[] PROGMEM = \"%s\";" % (i, v.get('color', "#333" if v.get('mode','') == 'OUTPUT' else "limegreen")))
        return '\\\n%s' % ' \\\n'.join(l) if l else ''

    @property
    def TARGET_GPIO_COUNT(self):
        jq = self.jq('.board.target', [])
        return '%s' % len(jq)

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
    def board_type(self):
        board = self.jq('.board.type', 'generic')
        if board == 'generic':
            board = 'esp_wroom_02'
        return board

    @property
    def board_mcu(self):
        if self.board_type == 'esp32dev':
            return 'esp32'
        return 'esp8266'

    @property
    def board_flash_size(self):
        fsize = str(self.jq('.board.flash_size', 4))
        return int(''.join(filter(str.isdigit, fsize)))

    @property
    def platform(self):
        if self.board_mcu == 'esp32':
            return 'https://github.com/platformio/platform-espressif32'
        return 'https://github.com/platformio/platform-espressif8266.git'

    @property
    def exception_decoder(self):
        if self.board_mcu == 'esp32':
            decoder = 'esp32_exception_decoder, default'
        elif self.board_mcu == 'esp8266':
            decoder = 'esp8266_exception_decoder, default'
        else:
            decoder = 'direct'
        return self.jq('.debug.exception_decoder', decoder)

    @property
    def build_type(self):
        return self.jq('.build.type', 'release')

    @property
    def build_ldscript(self):
        # Stupid assumptions:
        ldscript = "eagle.flash.4m.ld"
        if self.board_flash_size >= 16:
            ldscript = "eagle.flash.16m15m.ld"
        elif self.board_flash_size >= 8:
            ldscript = "eagle.flash.8m7m.ld"
        elif self.board_flash_size >= 4:
            ldscript = "eagle.flash.4m.ld"
        elif self.board_flash_size >= 2:
            ldscript = "eagle.flash.2m.ld"
        return self.jq('.build.ldscript', ldscript)

    @property
    def build_legacy_lib(self):
        return self.jq('.build.legacy_lib', False)

    @property
    def cpu_freq(self):
        default = 240000000 if self.board_mcu == 'esp32' else 160000000
        freq = self.jq('.board.cpu_freq', default) or default
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
    def serial_debug_baud(self):
        return self.jq('.uart.uart_debug.baud', 115200)

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
    def __init__(self, config: str, yml: dict, debug: bool = False):
        self.config_file = config
        self.config_name = os.path.splitext(os.path.basename(config))[0]
        self.builder_dir = os.path.join(git_toplevel_dir(), ".builder", self.config_name)
        self.cfg = yml
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
        cwd = os.path.abspath(os.path.dirname(__file__))

        print(f"Generate '{header_out.lstrip(cwd)}'")
        with open(header_out, "w") as f:
            header_tpl \
                .stream(cfg=self.header_extr) \
                .dump(f)

        print(f"Generate '{pio_out.lstrip(cwd)}'")
        with open(pio_out, "w") as f:
            pio_tpl \
                .stream(cfg=self.pio_extr) \
                .dump(f)

        if self.pio_extr.board_mcu == 'esp32':
            partitions_out = os.path.join(target_dir, "partitions_esp32.csv")
            self.gen_partitions(self.pio_extr.board_flash_size, partitions_out)

    def gen_partitions(self, flash_size_mb: int, fp: str):
        ALIGN = 0x10000  # alignment 64 KB
        def align(val): return (val + ALIGN - 1) & ~(ALIGN - 1)

        flash_size = flash_size_mb * 1024 * 1024

        # NVS minimum 20 KB, standard offset
        nvs_offset = 0x9000
        nvs_size   = 0x5000  # 20 KB

        # OTA Data: must be 0x2000 (8 KB)
        otadata_offset = 0xe000
        otadata_size   = 0x2000

        # App0:
        # We leave the alignment at 64 KB.
        app0_offset = 0x10000
        remaining = flash_size - app0_offset

        # Divide the rest of the flash into two equal parts.
        half = remaining // 2
        app0_size = align(half)
        app1_offset = app0_offset + app0_size
        app1_size = align(remaining - app0_size)

        partitions = [
            ("nvs",      "data", "nvs",      nvs_offset, nvs_size),
            ("otadata",  "data", "ota",      otadata_offset, otadata_size),
            ("app0",     "app",  "ota_0",    app0_offset, app0_size),
            ("app1",     "app",  "ota_1",    app1_offset, app1_size)
        ]

        lines = ["# Name, Type, SubType, Offset, Size, Flags"]
        for p in partitions:
            line = ", ".join(f"0x{x:X}" if isinstance(x, int) else str(x) for x in p)
            lines.append(line)

        with open(fp, "w") as f:
            f.write("\n".join(lines) + "\n")

        cwd = os.path.abspath(os.path.dirname(__file__))
        print(f"Generate '{fp.lstrip(cwd)}' for ESP32 and {flash_size_mb}MB flash size.")
        print(f"App0: 0x{app0_offset:X} - 0x{app0_offset + app0_size:X} ({app0_size} bytes)")
        print(f"App1: 0x{app1_offset:X} - 0x{app1_offset + app1_size:X} ({app1_size} bytes)")

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


def load_config(path: str) -> dict:
    with open(path) as f:
        return yaml.load(f, YamlLoader)


def main():
    debug = False
    configs = {}
    allconfigs = False

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
            path = os.path.join(git_toplevel_dir(), "configs/config.yml.example")
            configs[path] = load_config(path)
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
        configs[config] = load_config(config)

    if action in ('prepare', 'build', 'upload') and len(configs) == 0:
        allconfigs = True
        cfgdir = os.path.join(git_toplevel_dir(), "configs")
        for file in os.listdir(cfgdir):
            path = os.path.join(cfgdir, file)
            if file.endswith(".yml") and os.path.isfile(path):
                configs[path] = load_config(path)

    errors = {}

    for config, yml in configs.items():
        if allconfigs and yml.get('ignore_unless_requested', False):
            continue
        builder = Builder(config, yml, debug)
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
            errors[builder.config_name] = traceback.format_exc()
        print()

    if len(errors) > 0:
        print(COLOR_NOK + "FAILED BUILDS - Recap" + RESET_COLOR)
        print()
        for name, exc in errors.items():
            print(COLOR_NOK + f"+ {name}" + RESET_COLOR)
            print()
            print(textwrap.indent(exc, ' ' * 4) + '\n')
    else:
        print()
        print(COLOR_OK + "ALL BUILDS SUCCEEDED" + RESET_COLOR)


if __name__ == "__main__":
    main()
